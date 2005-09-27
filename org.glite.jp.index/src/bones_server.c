#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "glite/lb/srvbones.h"
#include "glite/security/glite_gss.h"

#include <stdsoap2.h>
#include "glite/security/glite_gsplugin.h"

#include "conf.h"
#include "db_ops.h"
#include "soap_ps_calls.h"

#include "soap_version.h"
#include "jpis_H.h"

#if GSOAP_VERSION <= 20602
#define soap_call___jpsrv__FeedIndex soap_call___ns1__FeedIndex
#define soap_call___jpsrv__FeedIndexRefresh soap_call___ns1__FeedIndexRefresh
#endif

#define CONN_QUEUE		20
#define MAX_SLAVES_NUM		20	// max. of slaves to be spawned
#define USER_QUERY_SLAVES_NUM	2	// # of slaves reserved for user queries if
					// # PS to conntact is << MAX_SLAVES_NUM

extern SOAP_NMAC struct Namespace jpis__namespaces[],jpps__namespaces[];
extern SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };
// namespaces[] not used here, but need to prevent linker to complain...

extern void MyFeedIndex(glite_jpis_context_t ctx, glite_jp_is_conf *conf, long int uniqueid, char *dest);

static int newconn(int,struct timeval *,void *);
static int request(int,struct timeval *,void *);
static int reject(int);
static int disconn(int,struct timeval *,void *);
static int data_init(void **data);

static struct glite_srvbones_service stab = {
	"JP Index Server", -1, newconn, request, reject, disconn
};

typedef struct {
	glite_jpis_context_t ctx;
	struct soap *soap;
} slave_data_t;

static time_t 		cert_mtime;
static char 		*server_cert, *server_key, *cadir;
static gss_cred_id_t 	mycred = GSS_C_NO_CREDENTIAL;
static char 		*mysubj;

static char 		*port = "8902";
static int 		debug = 1;

static glite_jp_context_t	ctx;
static char 			*glite_jp_default_namespace;
static glite_jp_is_conf		*conf;	// Let's make configuration visible to all slaves


int main(int argc, char *argv[])
{
	int			one = 1,opt,i;
	edg_wll_GssStatus	gss_code;
	struct sockaddr_in	a;
	char			*config_file;
	glite_jpis_context_t	isctx;


	glite_jp_init_context(&ctx);
	if (glite_jpis_init_context(&isctx, ctx) != 0) {
		fprintf(stderr, "Connect DB failed: %s (%s)\n", ctx->error->desc, ctx->error->source);
		glite_jp_free_context(ctx);
		return 1;
	}

	/* Read config options/file */
	// XXX: need add something meaningfull to src/conf.c !
	config_file = NULL;
	glite_jp_get_conf(argc, argv, config_file, &conf);

	/* XXX preliminary support for plugins 
	for (i=0; conf->plugins[i]; i++)
		glite_jp_typeplugin_load(ctx,conf->plugins[i]);
	*/
	

	if (glite_jpis_dropDatabase(ctx) != 0) {
		fprintf(stderr, "Drop DB failed: %s (%s)\n", ctx->error->desc, ctx->error->source);
		glite_jpis_free_context(isctx);
		glite_jp_free_context(ctx);
		return 1;
	}

	if (glite_jpis_initDatabase(ctx, conf) != 0) {
		fprintf(stderr, "Init DB failed: %s (%s)\n", ctx->error->desc, ctx->error->source);
		glite_jpis_free_context(isctx);
		glite_jp_free_context(ctx);
		return 1;
	}


#if GSOAP_VERSION <= 20602
	for (i=0; jpis__namespaces[i].id && strcmp(jpis__namespaces[i].id,"ns1"); i++);
#else
	for (i=0; jpis__namespaces[i].id && strcmp(jpis__namespaces[i].id,"jpsrv"); i++);
#endif
	assert(jpis__namespaces[i].id);
	glite_jp_default_namespace = jpis__namespaces[i].ns;

	stab.conn = socket(PF_INET, SOCK_STREAM, 0);
	if (stab.conn < 0) {
		perror("socket");
		return 1;
	}

	setsockopt(stab.conn,SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	a.sin_family = AF_INET;
	a.sin_addr.s_addr = INADDR_ANY;
	a.sin_port = htons(atoi(port));
	if (bind(stab.conn,(struct sockaddr *) &a, sizeof(a)) ) {
		char	buf[200];

		snprintf(buf,sizeof(buf),"bind(%d)",atoi(port));
		perror(buf);
		return 1;
	}

	if (listen(stab.conn,CONN_QUEUE)) {
		perror("listen()");
		return 1;
	}

	if (!server_cert || !server_key)
		fprintf(stderr, "%s: WARNING: key or certificate file not specified, "
				"can't watch them for changes\n",
				argv[0]);

	if ( cadir ) setenv("X509_CERT_DIR", cadir, 1);
	edg_wll_gss_watch_creds(server_cert, &cert_mtime);

	if ( !edg_wll_gss_acquire_cred_gsi(server_cert, server_key, &mycred, &mysubj, &gss_code)) 
		fprintf(stderr,"Server idenity: %s\n",mysubj);
	else fputs("WARNING: Running unauthenticated\n",stderr);

	/* daemonise */

	/* XXX: uncomment after testing phase
	for (i=0; conf->PS_list[i]; i++);	// count PS we need to contact
	i += USER_QUERY_SLAVES_NUM;		// add some slaves for user queries
	if (i > MAX_SLAVES_NUM)
		glite_srvbones_set_param(GLITE_SBPARAM_SLAVES_COUNT, MAX_SLAVES_NUM);
	else
		glite_srvbones_set_param(GLITE_SBPARAM_SLAVES_COUNT, i);
	*/
	/* for dbg - one slave OK */ glite_srvbones_set_param(GLITE_SBPARAM_SLAVES_COUNT,1);
	glite_srvbones_run(data_init,&stab,1 /* XXX: entries in stab */,debug);


	glite_jp_free_conf(conf);
	glite_jpis_free_context(isctx);
	glite_jp_free_context(ctx);

	return 0;
}

/* slave's init comes here */	
static int data_init(void **data)
{
	slave_data_t	*private;
	char		*PS_URL = NULL;
	long int	uniqueid;

	private = calloc(sizeof(*private), 1);
	if (glite_jpis_init_context(&private->ctx, ctx) != 0) {
		printf("[%d] slave_init(): DB error: %s (%s)\n",getpid(),ctx->error->desc,ctx->error->source);
		return -1;
	}
	private->soap = soap_new();
	printf("[%d] slave started\n",getpid());

	/* ask PS server for data */
	do {
		switch (glite_jpis_lockUninitializedFeed(private->ctx,&uniqueid,&PS_URL)) {
			case ENOENT:
				// no more feeds to initialize
				return 0;
			case ENOLCK:
				// error during locking
				printf("[%d] slave_init(): Locking error.\n",getpid());
				free(PS_URL);
				glite_jpis_free_context(private->ctx);
				return -1;
			default:
				// contact PS server, ask for data, save feedId and expiration
				// to DB and unlock feed
				MyFeedIndex(private->ctx, conf, uniqueid, PS_URL);
				free(PS_URL);
				PS_URL = NULL;
				break;
		}
	} while (1);
}

static int newconn(int conn,struct timeval *to,void *data)
{
	slave_data_t     *private = (slave_data_t *)data;
	struct soap		*soap = private->soap;
	glite_jp_context_t	ctx = private->ctx->jpctx;
	glite_gsplugin_Context	plugin_ctx;

	gss_cred_id_t		newcred = GSS_C_NO_CREDENTIAL;
	edg_wll_GssStatus	gss_code;
	gss_name_t		client_name = GSS_C_NO_NAME;
	gss_buffer_desc		token = GSS_C_EMPTY_BUFFER;
	OM_uint32		maj_stat,min_stat;
	int			ret = 0;


	soap_init2(soap,SOAP_IO_KEEPALIVE,SOAP_IO_KEEPALIVE);
	soap_set_namespaces(soap,jpis__namespaces);

/* not yet: client to JP Storage Server
 * probably wil come to other place, just not forget it....
	ctx->other_soap = soap_new();
	soap_init(ctx->other_soap);
	soap_set_namespaces(ctx->other_soap,jpps__namespaces);
*/


	glite_gsplugin_init_context(&plugin_ctx);
	plugin_ctx->connection = calloc(1,sizeof *plugin_ctx->connection);
	soap_register_plugin_arg(soap,glite_gsplugin,plugin_ctx);

/* now we probably no not play AAA game, but useful in future */
	switch (edg_wll_gss_watch_creds(server_cert,&cert_mtime)) {
		case 0: break;
		case 1: if (!edg_wll_gss_acquire_cred_gsi(server_cert,server_key,
						&newcred,NULL,&gss_code))
			{

				printf("[%d] reloading credentials\n",getpid()); /* XXX: log */
				gss_release_cred(&min_stat,&mycred);
				mycred = newcred;
			}
			break;
		case -1:
			printf("[%d] edg_wll_gss_watch_creds failed\n", getpid()); /* XXX: log */
			break;
	}

	/* TODO: DNS paranoia etc. */

	if (edg_wll_gss_accept(mycred,conn,to,plugin_ctx->connection,&gss_code)) {
		printf("[%d] GSS connection accept failed, closing.\n", getpid());
		ret = 1;
		goto cleanup;
	}

	maj_stat = gss_inquire_context(&min_stat,plugin_ctx->connection->context,
			&client_name, NULL, NULL, NULL, NULL, NULL, NULL);

	if (!GSS_ERROR(maj_stat))
		maj_stat = gss_display_name(&min_stat,client_name,&token,NULL);

	if (ctx->peer) free(ctx->peer);
	if (!GSS_ERROR(maj_stat)) {
		printf("[%d] client DN: %s\n",getpid(),(char *) token.value); /* XXX: log */

		ctx->peer = strdup(token.value);
		memset(&token, 0, sizeof(token));
	}
	else {
		printf("[%d] annonymous client\n",getpid());
		ctx->peer = NULL;
	}

	if (client_name != GSS_C_NO_NAME) gss_release_name(&min_stat, &client_name);
	if (token.value) gss_release_buffer(&min_stat, &token);

	return 0;

cleanup:
	glite_jpis_free_context(private->ctx);
	glite_gsplugin_free_context(plugin_ctx);
	soap_end(soap);

	return ret;
}

static int request(int conn,struct timeval *to,void *data)
{
	slave_data_t		*private = (slave_data_t *)data;
	struct soap		*soap = private->soap;
	glite_jp_context_t	ctx = private->ctx->jpctx;

	glite_gsplugin_set_timeout(glite_gsplugin_get_context(soap),to);

	soap->max_keep_alive = 1;	/* XXX: prevent gsoap to close connection */ 
	soap_begin(soap);
	if (soap_begin_recv(soap)) {
		if (soap->error < SOAP_STOP) {
			soap_send_fault(soap);
			return EIO;
		}
		return ENOTCONN;
	}

	soap->keep_alive = 1;
	if (soap_envelope_begin_in(soap)
		|| soap_recv_header(soap)
		|| soap_body_begin_in(soap)
		|| jpis__serve_request(soap)
#if GSOAP_VERSION >= 20700
		|| (soap->fserveloop && soap->fserveloop(soap))
#endif
	)
	{
		soap_send_fault(soap);
		if (ctx->error) {
			/* XXX: shall we die on some errors? */
			int	err = ctx->error->code;
			glite_jp_clear_error(ctx);
			return err;
		}
		return 0;
	}

	glite_jp_run_deferred(ctx);
	return 0;
}

static int reject(int conn)
{
	int	flags = fcntl(conn, F_GETFL, 0);

	fcntl(conn,F_SETFL,flags | O_NONBLOCK);
	edg_wll_gss_reject(conn);

	return 0;
}

static int disconn(int conn,struct timeval *to,void *data)
{
	slave_data_t		*private = (slave_data_t *)data;
	struct soap		*soap = private->soap;

	glite_jpis_free_context(private->ctx);
	soap_end(soap); // clean up everything and close socket
	
	return 0;
}


