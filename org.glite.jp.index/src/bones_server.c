#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>

#include <glite/jp/types.h>
#include <glite/jp/context.h>

#include <glite/lbu/srvbones.h>

#include <stdsoap2.h>
#include <glite/security/glite_gss.h>
#include <glite/security/glite_gsplugin.h>

#include "conf.h"
#include "db_ops.h"
#include "soap_ps_calls.h"
#include "context.h"
#include "common.h"

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
#define REACTION_TIME		60*2	// when try reconnect to PS in case of new feeds (in sec)
#define LAUNCH_TIME		2	// wait (for starting slaves) before requesting feeds


extern SOAP_NMAC struct Namespace jp__namespaces[],jpps__namespaces[];

int newconn(int,struct timeval *,void *);
int request(int,struct timeval *,void *);
static int reject(int);
static int disconn(int,struct timeval *,void *);
int data_init(void **data);
#ifndef ONETIME_FEEDS
int feed_loop_slave(void);
#endif


static struct glite_srvbones_service stab = {
	"JP Index Server", -1, newconn, request, reject, disconn
};

static time_t 		cert_mtime;
static char 		*server_cert, *server_key, *cadir;
static edg_wll_GssCred 	mycred = NULL;

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
	int retval = 0;
	char *err;

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
	if (!conf->debug) glite_srvbones_daemonize("glite-jp-indexd", conf->pidfile, conf->logfile);

	/* XXX preliminary support for plugins 
	for (i=0; conf->plugins[i]; i++)
		glite_jp_typeplugin_load(ctx,conf->plugins[i]);
	*/
	

	if (conf->delete_db) {
		if (glite_jpis_dropDatabase(isctx) != 0) {
			fprintf(stderr, "Drop DB failed: ");
			retval = 1;
			goto quit;
		}
	}

	if (glite_jpis_initDatabase(isctx) != 0) {
		fprintf(stderr, "Init DB failed: ");
		retval = 1;
		goto quit;
	}

	server_cert = conf->server_cert;
	server_key = conf->server_key;

	if (!server_cert || !server_key)
		fprintf(stderr, "%s: WARNING: key or certificate file not specified, "
				"can't watch them for changes\n",
				argv[0]);

	if ( cadir ) setenv("X509_CERT_DIR", cadir, 1);
	edg_wll_gss_watch_creds(server_cert, &cert_mtime);

	if ( !edg_wll_gss_acquire_cred_gsi(server_cert, server_key, &mycred, &gss_code)) 
		fprintf(stderr,"Server identity: %s\n",mycred ? mycred->name : "NULL");
	else fputs("WARNING: Running unauthenticated\n",stderr);

	if (conf->feeding) {
		fprintf(stderr, "%s: Feeding from '%s'\n", argv[0], conf->feeding);
		retval = glite_jpis_feeding(isctx, conf->feeding, mycred ? mycred->name : NULL);
		goto quit;
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
#ifndef ONETIME_FEEDS
 	if (feed_loop_slave() < 0) {
 		fprintf(stderr, "forking feed_loop_slave failed!\n");
	} else
#endif
	glite_srvbones_run(data_init,&stab,1 /* XXX: entries in stab */,debug);

quit:
	if (isctx->jpctx->error) {
		err = glite_jp_error_chain(isctx->jpctx);
		fprintf(stderr, "%s: %s\n", argv[0], err);
		free(err);
	}

	glite_jpis_free_db(isctx);
	glite_jp_free_conf(conf);
	glite_jpis_free_context(isctx);
	glite_jp_free_context(ctx);

	return retval;
}


static int get_soap(struct soap *soap, glite_jpis_context_t ctx) {
	glite_gsplugin_Context			plugin_ctx;

	glite_gsplugin_init_context(&plugin_ctx);
	
	soap_init(soap);
	soap_set_namespaces(soap, jp__namespaces);
	soap_set_omode(soap, SOAP_IO_BUFFER);   // set buffered response
                                                // buffer set to SOAP_BUFLEN (default = 8k)	
	if (soap_register_plugin_arg(soap,glite_gsplugin,plugin_ctx))
		return glite_jpis_stack_error(ctx->jpctx, EIO, "can't register gsoap plugin");

	return 0;
}


/* looking for some feed in DB */
static int feed_caller(struct soap *soap, glite_jpis_context_t isctx) {
	char *PS_URL, *feedid, *errs;
	long int uniqueid;
	int i, ok, ret, status, initialized, result = 0;

	// dirty hack - try quicker several times first
	glite_jp_clear_error(isctx->jpctx);

	feedid = NULL;
	for (initialized = 0; initialized <= 1; initialized++) {
		switch (glite_jpis_lockSearchFeed(isctx,initialized,&uniqueid,&PS_URL,&status,&feedid)) {
		case 0:
			// some locked feeds found
			ok = 0;
			for (i = 0; i < 10; i++) {
				if (!initialized) {
					// contact PS server, ask for data, save
					// feedId and expiration to DB and unlock the feed
					ret = MyFeedIndex(soap, isctx, uniqueid, PS_URL);
				} else {
					ret = MyFeedRefresh(soap, isctx, uniqueid, PS_URL, status, feedid);
				}
				if (ret) {
					// error when connecting to PS
					errs = glite_jp_error_chain(isctx->jpctx);
					printf("[%d] %s: %s, reconnecting later\n", getpid(), __FUNCTION__, errs);
					free(errs);
				} else {
					lprintf("%s %s (%ld) ok\n", initialized ? "refresh" : "init", feedid, uniqueid);
					ok = 1;
					break;
				}
			}
			if (!ok) {
				// when unintialized feed: always reconnect
				// when not refreshed feed: reconnect only once and two times quicker
				if (!initialized || (status & GLITE_JP_IS_STATE_ERROR) == 0) {
					lprintf("reconnecting %s (%ld)\n", feedid, uniqueid);
					glite_jpis_tryReconnectFeed(isctx, uniqueid, time(NULL) + RECONNECT_TIME / (initialized  + 1), status | GLITE_JP_IS_STATE_ERROR);
				} else {
					lprintf("destroying %s (%ld)\n", feedid, uniqueid);
					glite_jpis_destroyTryReconnectFeed(isctx, uniqueid, time(NULL) - 1);
				}
			}
			free(PS_URL); PS_URL = NULL;
			free(feedid); feedid = NULL;

			sleep(RECONNECT_TIME_QUICK);

			result = 1;
			break;
		case ENOENT:
			// no more feeds to initialize
			break;
		default:
			// error during locking
			printf("[%d] %s: Locking error: ", getpid(), __FUNCTION__);
			if (isctx->jpctx->error) {
				errs = glite_jp_error_chain(isctx->jpctx);
				printf("%s\n", errs);
				free(errs);
			} else printf("(no detail)\n");
			return -1;
		}
	}

	return result;
}


#ifndef ONETIME_FEEDS
int feed_loop_slave(void) {
	pid_t pid;
	glite_jpis_context_t isctx;
	struct soap soap;
	char *errs;

	if ( (pid = fork()) ) return pid;

	glite_jpis_init_context(&isctx, ctx, conf);
	if (glite_jpis_init_db(isctx) != 0) {
		printf("[%d] %s: DB error: %s (%s)\n", getpid(), __FUNCTION__, ctx->error->desc, ctx->error->source);
		exit(1);
	}

	if (get_soap(&soap, isctx) != 0) {
		printf("[%d] %s: ", getpid(), __FUNCTION__);
		if (isctx->jpctx->error) {
			errs = glite_jp_error_chain(isctx->jpctx);
			printf("%s\n", errs);
			free(errs);
		} else printf("(no detail)\n");
		exit(1);
	}

	printf("[%d] %s: waiting before feed requests...\n", getpid(), __FUNCTION__);
	sleep(LAUNCH_TIME);
	printf("[%d] %s: feeder slave started\n", getpid(), __FUNCTION__);
	do {
		switch (feed_caller(&soap, isctx)) {
			case 1: break;
			case 0:
				sleep(REACTION_TIME);
				break;
			default:
				if (isctx->jpctx->error) {
					errs = glite_jp_error_chain(isctx->jpctx);
					printf("[%d] %s: %s\n", getpid(), __FUNCTION__, errs);
					free(errs);
				}
				printf("[%d] %s: feed locking error, slave terminated\n", getpid(), __FUNCTION__);
				exit(1);
		}
	} while (1);

	printf("[%d] %s: slave terminated\n", getpid(), __FUNCTION__);
	exit(0);
}
#endif


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

#if ONETIME_FEEDS
	if (get_soap(private->soap, ctx) != 0) {
		printf("[%d] %s: ", getpid(), __FUNCTION__);
		if (isctx->jpctx->error) {
			errs = glite_jp_error_chain(ctx->jpctx);
			printf("%s\n", errs);
			free(errs);
		} else printf("(no error)\n");
		exit(1);
	}

	/* ask PS server for data */
	do {
		switch (feed_caller(private->soap, private->ctx)) {
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
#else
	*data = (void *) private;
	return 0;
#endif
}


int newconn(int conn,struct timeval *to,void *data)
{
	slave_data_t     *private = (slave_data_t *)data;
	struct soap		*soap = private->soap;
	glite_jp_context_t	ctx = private->ctx->jpctx;
	glite_gsplugin_Context	plugin_ctx;

	edg_wll_GssCred		newcred = NULL;
	edg_wll_GssStatus	gss_code;
	int			ret = 0;
	edg_wll_GssPrincipal	client = NULL;
	edg_wll_GssConnection	connection;


	soap_init2(soap,SOAP_IO_KEEPALIVE,SOAP_IO_KEEPALIVE);
	soap_set_omode(soap, SOAP_IO_BUFFER);	// set buffered response
						// buffer set to SOAP_BUFLEN (default = 8k)
	soap_set_namespaces(soap,jp__namespaces);
	soap->user = (void *) private;

	switch (edg_wll_gss_watch_creds(server_cert,&cert_mtime)) {
		case 0: break;
		case 1: if (!edg_wll_gss_acquire_cred_gsi(server_cert,server_key,
						&newcred,&gss_code))
			{

				printf("[%d] reloading credentials\n",getpid()); /* XXX: log */
				edg_wll_gss_release_cred(&mycred, NULL);
				mycred = newcred;
			}
			break;
		case -1:
			printf("[%d] edg_wll_gss_watch_creds failed\n", getpid()); /* XXX: log */
			break;
	}

	/* TODO: DNS paranoia etc. */
	memset(&connection, 0, sizeof(connection));
	if (edg_wll_gss_accept(mycred,conn,to,&connection,&gss_code)) {
		char	*et;

		edg_wll_gss_get_error(&gss_code,"",&et);

		fprintf(stderr,"[%d] GSS connection accept failed: %s\nClosing connection.\n",getpid(),et);
		free(et);
		ret = 1;
		soap_end(soap);
		return 1;
	}

        ret = edg_wll_gss_get_client_conn(&connection, &client, NULL);

	if (ctx->peer) free(ctx->peer);
	if (ret || client->flags & EDG_WLL_GSS_FLAG_ANON) {
		printf("[%d] annonymous client\n",getpid());
		ctx->peer = NULL;
	}
	else {
		printf("[%d] client DN: %s\n",getpid(),client->name); /* XXX: log */

		ctx->peer = strdup(client->name);
		edg_wll_gss_free_princ(client);
	}

	glite_gsplugin_init_context(&plugin_ctx);
	glite_gsplugin_set_connection(plugin_ctx, &connection);
	soap_register_plugin_arg(soap,glite_gsplugin,plugin_ctx);

	return 0;
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
			return err == EIO ? -err : err;		/* EIO is fatal */
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


