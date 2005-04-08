#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "glite/lb/srvbones.h"
#include "glite/security/glite_gss.h"

#include <stdsoap2.h>
#include "glite/security/glite_gsplugin.h"



#include "jpps_H.h"

#define CONN_QUEUE	20

extern SOAP_NMAC struct Namespace jpis__namespaces[],jpps__namespaces[];

static int newconn(int,struct timeval *,void *);
static int request(int,struct timeval *,void *);
static int reject(int);
static int disconn(int,struct timeval *,void *);
static int data_init(void **data);

static struct glite_srvbones_service stab = {
	"JP Primary Storage", -1, newconn, request, reject, disconn
};

static time_t cert_mtime;
static char *server_cert, *server_key, *cadir;
static gss_cred_id_t mycred = GSS_C_NO_CREDENTIAL;
static char *mysubj;

static char *port = "8901";
static int debug = 1;

static glite_jp_context_t	ctx;

int main(int argc, char *argv[])
{
	int	one = 1;
	edg_wll_GssStatus	gss_code;
	struct sockaddr_in	a;

	/* XXX: read options */

	glite_jp_init_context(&ctx);

	if (glite_jppsbe_init(ctx, &argc, argv)) {
		/* XXX log */
		fputs(glite_jp_error_chain(ctx), stderr);
		exit(1);
	}

	if (glite_jpps_fplug_load(ctx, &argc, argv)) {
		/* XXX log */
		fputs(glite_jp_error_chain(ctx), stderr);
		exit(1);
	}

	srand48(time(NULL)); /* feed id generation */

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
		fprintf(stderr, "%s: WARNING: key or certificate file not specified,"
				"can't watch them for changes\n",
				argv[0]);

	if ( cadir ) setenv("X509_CERT_DIR", cadir, 1);
	edg_wll_gss_watch_creds(server_cert, &cert_mtime);

	if ( !edg_wll_gss_acquire_cred_gsi(server_cert, server_key, &mycred, &mysubj, &gss_code)) 
		fprintf(stderr,"Server idenity: %s\n",mysubj);
	else fputs("WARNING: Running unauthenticated\n",stderr);

	/* XXX: daemonise */

	glite_srvbones_set_param(GLITE_SBPARAM_SLAVES_COUNT,1);
	glite_srvbones_run(data_init,&stab,1 /* XXX: entries in stab */,debug);

	return 0;
}

static int data_init(void **data)
{
	*data = (void *) soap_new();

	printf("[%d] slave started\n",getpid());

	return 0;
}

static int newconn(int conn,struct timeval *to,void *data)
{
	struct soap	*soap = (struct soap *) data;
	glite_gsplugin_Context	plugin_ctx;

	gss_cred_id_t		newcred = GSS_C_NO_CREDENTIAL;
	edg_wll_GssStatus	gss_code;
	gss_name_t		client_name = GSS_C_NO_NAME;
	gss_buffer_desc		token = GSS_C_EMPTY_BUFFER;
	OM_uint32		maj_stat,min_stat;


	int	ret = 0;

	soap_init2(soap,SOAP_IO_KEEPALIVE,SOAP_IO_KEEPALIVE);
	soap_set_namespaces(soap,jpps__namespaces);
	soap->user = (void *) ctx; /* XXX: one instance per slave */

/* not yet: client to JP index
	ctx->other_soap = soap_new();
	soap_init(ctx->other_soap);
	soap_set_namespaces(ctx->other_soap,jpis__namespaces);
*/


	glite_gsplugin_init_context(&plugin_ctx);
	plugin_ctx->connection = calloc(1,sizeof *plugin_ctx->connection);
	soap_register_plugin_arg(soap,glite_gsplugin,plugin_ctx);

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
	glite_gsplugin_free_context(plugin_ctx);
	soap_end(soap);

	return ret;
}

static int request(int conn,struct timeval *to,void *data)
{
	struct soap		*soap = data;
	glite_jp_context_t	ctx = soap->user;

	glite_gsplugin_set_timeout(glite_gsplugin_get_context(soap),to);

/* FIXME: does not work, ask nykolas */
	soap->max_keep_alive = 1;	/* XXX: prevent gsoap to close connection */ 
	soap_begin(soap);
	if (soap_begin_recv(soap)) {
		if (soap->error < SOAP_STOP) {
			soap_send_fault(soap);
			return -1;
		}
		return ENOTCONN;
	}

	if (soap_envelope_begin_in(soap)
		|| soap_recv_header(soap)
		|| soap_body_begin_in(soap)
		|| jpps__serve_request(soap)
#if GSOAP_VERSION >= 20700
		|| (soap->fserveloop && soap->fserveloop(soap))
#endif
	)
	{
		soap_send_fault(soap);
		return -1;
	}

	glite_jp_run_deferred(ctx);
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
	struct soap	*soap = (struct soap *) data;
	soap_end(soap); // clean up everything and close socket

	return 0;
}


/* XXX: we don't use it */
SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };
