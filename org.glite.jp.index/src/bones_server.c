#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>

#include <glite/jp/types.h>
#include <glite/jp/context.h>

#include <glite/lb/srvbones.h>

#include <stdsoap2.h>
#include <glite/security/glite_gss.h>
#include <glite/security/glite_gsplugin.h>

#include "conf.h"
#include "db_ops.h"
#include "soap_ps_calls.h"
#include "context.h"
#include "common_server.h"

#include "soap_version.h"
#include "jp_H.h"
#include "jp_.nsmap"

#if GSOAP_VERSION <= 20602
#define soap_call___jpsrv__FeedIndex soap_call___ns1__FeedIndex
#define soap_call___jpsrv__FeedIndexRefresh soap_call___ns1__FeedIndexRefresh
#endif

#define CONN_QUEUE		20
#define MAX_SLAVES_NUM		20	// max. of slaves to be spawned
#define USER_QUERY_SLAVES_NUM	2	// # of slaves reserved for user queries if
					// # PS to conntact is << MAX_SLAVES_NUM

#define RECONNECT_TIME		60*20	// when try reconnect to PS in case of error (in sec)
#define RECONNECT_TIME_QUICK	1	// time between feed requests


extern SOAP_NMAC struct Namespace jp__namespaces[],jpps__namespaces[];

int newconn(int,struct timeval *,void *);
int request(int,struct timeval *,void *);
static int reject(int);
static int disconn(int,struct timeval *,void *);
int data_init(void **data);


static struct glite_srvbones_service stab = {
	"JP Index Server", -1, newconn, request, reject, disconn
};

static time_t 		cert_mtime;
static char 		*server_cert, *server_key, *cadir;
static gss_cred_id_t 	mycred = GSS_C_NO_CREDENTIAL;
static char 		*mysubj;

static char 		*port = GLITE_JPIS_DEFAULT_PORT_STR;
static int 		debug = 1;

static glite_jp_context_t	ctx;
static glite_jp_is_conf		*conf;	// Let's make configuration visible to all slaves


int main(int argc, char *argv[])
{
	int			one = 1, nfeeds;
	edg_wll_GssStatus	gss_code;
	struct sockaddr_in	a;
	glite_jpis_context_t	isctx;


	glite_jp_init_context(&ctx);

	if (glite_jp_get_conf(argc, argv, &conf)) {
		glite_jp_free_context(ctx);
		exit(1);
	}
	glite_jpis_init_context(&isctx, ctx, conf);

	/* connect to DB */
	if (glite_jpis_init_db(isctx) != 0) {
		fprintf(stderr, "Connect DB failed: %s (%s)\n", ctx->error->desc, ctx->error->source);
		glite_jpis_free_context(isctx);
		glite_jp_free_context(ctx);
		glite_jp_free_conf(conf);
		return 1;
	}

	/* daemonize */
	if (!conf->debug) glite_jpis_daemonize("glite-jp-indexd", conf->pidfile, conf->logfile);

	/* XXX preliminary support for plugins 
	for (i=0; conf->plugins[i]; i++)
		glite_jp_typeplugin_load(ctx,conf->plugins[i]);
	*/
	

	if (glite_jpis_dropDatabase(isctx) != 0) {
		fprintf(stderr, "Drop DB failed: %s (%s)\n", ctx->error->desc, ctx->error->source);
		glite_jpis_free_db(isctx);
		glite_jpis_free_context(isctx);
		glite_jp_free_context(ctx);
		glite_jp_free_conf(conf);
		return 1;
	}

	if (glite_jpis_initDatabase(isctx) != 0) {
		fprintf(stderr, "Init DB failed: %s (%s)\n", ctx->error->desc, ctx->error->source);
		glite_jpis_free_db(isctx);
		glite_jpis_free_context(isctx);
		glite_jp_free_context(ctx);
		glite_jp_free_conf(conf);
		return 1;
	}

	stab.conn = socket(PF_INET, SOCK_STREAM, 0);
	if (stab.conn < 0) {
		perror("socket");
		return 1;
	}

	setsockopt(stab.conn,SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	if (conf->port) port = conf->port;
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

	server_cert = conf->server_cert;
	server_key = conf->server_key;

	if (!server_cert || !server_key)
		fprintf(stderr, "%s: WARNING: key or certificate file not specified, "
				"can't watch them for changes\n",
				argv[0]);

	if ( cadir ) setenv("X509_CERT_DIR", cadir, 1);
	edg_wll_gss_watch_creds(server_cert, &cert_mtime);

	if ( !edg_wll_gss_acquire_cred_gsi(server_cert, server_key, &mycred, &mysubj, &gss_code)) 
		fprintf(stderr,"Server idenity: %s\n",mysubj);
	else fputs("WARNING: Running unauthenticated\n",stderr);

 	// XXX: more tests needed
	if (conf->feeds)
	 	for (nfeeds=0; conf->feeds[nfeeds]; nfeeds++);
	else nfeeds = 0;
 	if (conf->slaves <= 0) {
 		// add some slaves for user queries and PS responses
 		conf->slaves = nfeeds + (USER_QUERY_SLAVES_NUM - 1);  
 		if (conf->slaves > MAX_SLAVES_NUM) conf->slaves = MAX_SLAVES_NUM;
	}
 	//
 	// SUM(PS, feeds(PS) - slaves(PS)) slaves would be blocked
 	// when waited for all PS
 	//
 	// wild guess for slaves(PS) == 1 on all PS:
 	// 1 + SUM(PS, feeds(PS) - 1) slaves is required,
 	// SUM(PS, feeds(PS)) is enough.
 	//
 	if (conf->slaves < nfeeds) {
 		fprintf(stderr, "WARNING: %d slaves can be too low for %d feeds\n", conf->slaves, nfeeds);
 	}
 	glite_srvbones_set_param(GLITE_SBPARAM_SLAVES_COUNT, conf->slaves);
	glite_srvbones_run(data_init,&stab,1 /* XXX: entries in stab */,debug);

	glite_jpis_free_db(isctx);
	glite_jp_free_conf(conf);
	glite_jpis_free_context(isctx);
	glite_jp_free_context(ctx);

	return 0;
}


/* looking for some feed in DB */
int feed_caller(glite_jpis_context_t isctx, glite_jp_is_conf *conf) {
	char *PS_URL;
	long int uniqueid;
	int i;

	// dirty hack - try quicker several times first
	glite_jp_clear_error(isctx->jpctx);
	switch (glite_jpis_lockUninitializedFeed(isctx,&uniqueid,&PS_URL)) {
		case 0: 
			for (i = 0; i < 10; i++) {
				// contact PS server, ask for data, save
				// feedId and expiration to DB and unlock feed
				if (MyFeedIndex(isctx, conf, uniqueid, PS_URL) != 0) {
					// error when connecting to PS
					printf("[%d] %s: %s (%s), reconnecting later\n", getpid(), __FUNCTION__, isctx->jpctx->error->desc, isctx->jpctx->error->source);
					if (i + 1 < 10) glite_jpis_unlockFeed(isctx, uniqueid);
					else glite_jpis_tryReconnectFeed(isctx, uniqueid, time(NULL) + RECONNECT_TIME);
				} else {
					free(PS_URL);
					break;
				}
			}
			sleep(RECONNECT_TIME_QUICK);

			return 1;
		case ENOENT:
			// no more feeds to initialize
			return 0;
		default:
			// error during locking
			printf("[%d] %s: Locking error: ", getpid(), __FUNCTION__);
			if (isctx->jpctx->error) printf("%s (%d)\n", isctx->jpctx->error->desc, isctx->jpctx->error->code);
			else printf("\n");
			return -1;
	}
}


/* slave's init comes here */	
int data_init(void **data)
{
	slave_data_t	*private;

	private = calloc(sizeof(*private), 1);
	glite_jpis_init_context(&private->ctx, ctx, conf);
	if (glite_jpis_init_db(private->ctx) != 0) {
		printf("[%d] slave_init(): DB error: %s (%s)\n",getpid(),ctx->error->desc,ctx->error->source);
		return -1;
	}

	printf("[%d] slave started\n",getpid());
	private->soap = soap_new();

	/* ask PS server for data */
	do {
		switch (feed_caller(private->ctx, conf)) {
			case 1:
				// one feed handled
				break;
			case 0:
				// no more feeds to initialize
				*data = (void *) private;
				return 0;
			default:
				// error during locking
				glite_jpis_free_db(private->ctx);
				glite_jpis_free_context(private->ctx);
				return -1;
		}
	} while (1);
}


int newconn(int conn,struct timeval *to,void *data)
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
	soap_set_omode(soap, SOAP_IO_BUFFER);	// set buffered response
						// buffer set to SOAP_BUFLEN (default = 8k)
	soap_set_namespaces(soap,jp__namespaces);
	soap->user = (void *) private;

	glite_gsplugin_init_context(&plugin_ctx);
	plugin_ctx->connection = calloc(1,sizeof *plugin_ctx->connection);

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
		char	*et;

		edg_wll_gss_get_error(&gss_code,"",&et);

		fprintf(stderr,"[%d] GSS connection accept failed: %s\nClosing connection.\n",getpid(),et);
		free(et);
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

	soap_register_plugin_arg(soap,glite_gsplugin,plugin_ctx);

	return 0;

cleanup:
	glite_gsplugin_free_context(plugin_ctx);
	soap_end(soap);

	return ret;
}

int request(int conn UNUSED,struct timeval *to,void *data)
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
		|| jp__serve_request(soap)
#if GSOAP_VERSION >= 20700
		|| (soap->fserveloop && soap->fserveloop(soap))
#endif
	)
	{
		soap_send_fault(soap);	// sets soap->keep_alive back to 0 :(
					// and closes connection
		if (ctx->error) {
			/* XXX: shall we die on some errors? */
			int	err = ctx->error->code;
			glite_jp_clear_error(ctx);
			return err;
		}

		return ECANCELED;	// let srv_bones know something is wrong					
	}

	glite_jp_run_deferred(ctx);
	return ENOTCONN;
}

static int reject(int conn)
{
	int	flags = fcntl(conn, F_GETFL, 0);

	fcntl(conn,F_SETFL,flags | O_NONBLOCK);
	edg_wll_gss_reject(conn);

	return 0;
}

static int disconn(int conn UNUSED,struct timeval *to UNUSED,void *data)
{
	slave_data_t		*private = (slave_data_t *)data;
	struct soap		*soap = private->soap;

// XXX: belongs to "data_init complement"
//	glite_jpis_free_db(private->ctx);
//	glite_jpis_free_context(private->ctx);
	soap_end(soap); // clean up everything and close socket
	
	return 0;
}


