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

#include "feed.h"
#include "backend.h"
#include "file_plugin.h"

#include "soap_version.h"
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
char *server_cert, *server_key, *cadir;
gss_cred_id_t mycred = GSS_C_NO_CREDENTIAL;
static char *mysubj;

static char *port = "8901";
static int debug = 0;

static glite_jp_context_t	ctx;

static int call_opts(glite_jp_context_t,char *,char *,int (*)(glite_jp_context_t,int,char **));

const char *glite_jp_default_namespace;

pid_t	master;

int main(int argc, char *argv[])
{
	int	one = 1,opt,i;
	edg_wll_GssStatus	gss_code;
	struct sockaddr_in	a;
	char	*b_argv[20] = { "backend" },*p_argv[20] = { "plugins" },*com;
	int	b_argc,p_argc;
	char	buf[1000];
	int	slaves = 10;
	char	*logfile = "/dev/null";
	char	pidfile[PATH_MAX] = "/var/run/glite-jp-primarystoraged.pid";
	FILE	*fpid;

	glite_jp_init_context(&ctx);
	globus_libc_gethostname(buf,sizeof buf);
	buf[999] = 0;
	ctx->myURL = buf;

	if (geteuid()) snprintf(pidfile,sizeof pidfile,"%s/glite-jp-primarystoraged.pid",getenv("HOME"));

	b_argc = p_argc = 1;

	while ((opt = getopt(argc,argv,"B:P:a:p:s:dl:i:c:k:n")) != EOF) switch (opt) {
		case 'B':
			assert(b_argc < 20);
			if (com = strchr(optarg,',')) *com = 0;
			
			/* XXX: memleak -- who cares for once */
			asprintf(&b_argv[b_argc++],"-%s",optarg);
			if (com) b_argv[b_argc++] = com+1;

			break;
		case 'P':
			assert(p_argc < 20);
			p_argv[p_argc++] = optarg;

			break;
		case 'a':
			if (glite_jpps_readauth(ctx,optarg)) {
				fprintf(stderr,"%s: %s\n",argv[0],glite_jp_error_chain(ctx));
				exit (1);
			}
			break;
		case 'p':
			port = optarg;
			break;
		case 'd': debug = 1; break;
		case 's': slaves = atoi(optarg);
			  if (slaves <= 0) {
				  fprintf(stderr,"%s: -s %s: invalid number\n",argv[0],optarg);
				  exit(1);
			  }
			  break;
		case 'l': logfile = optarg; break;
		case 'i': strncpy(pidfile,optarg,PATH_MAX); pidfile[PATH_MAX-1] = 0; break;
		case 'c': server_cert = optarg; break;
		case 'k': server_key = optarg; break;
		case 'n': ctx->noauth = 1; break;
		case '?': fprintf(stderr,"usage: %s: -Bb,val ... -Pplugin.so ...\n"
					  "b is backend option\n",argv[0]);
			  exit (1);
	}

	if (b_argc == 1) {
		fputs("-B required\n",stderr);
		exit (1);
	}
	
	optind = 0; /* XXX: getopt used internally */
	if (glite_jppsbe_init(ctx,b_argc,b_argv)) {
		fputs(glite_jp_error_chain(ctx), stderr);
		exit(1);
	}

	optind = 0; /* XXX: getopt used internally */
	if (b_argc > 1 && glite_jpps_fplug_load(ctx,p_argc,p_argv)) {
		fputs(glite_jp_error_chain(ctx), stderr);
		exit(1);
	}

	srand48(time(NULL)); /* feed id generation */

#if GSOAP_VERSION <= 20602
	for (i=0; jpps__namespaces[i].id && strcmp(jpps__namespaces[i].id,"ns1"); i++);
#else
	for (i=0; jpps__namespaces[i].id && strcmp(jpps__namespaces[i].id,"jpsrv"); i++);
#endif
	assert(jpps__namespaces[i].id);
	glite_jp_default_namespace = jpps__namespaces[i].ns;

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

	/* XXX: daemonise */

	if (!debug) {
		int	lfd = open(logfile,O_CREAT|O_TRUNC|O_WRONLY,0600);
		if (lfd < 0) {
			fprintf(stderr,"%s: %s: %s\n",argv[0],logfile,strerror(errno));
			exit(1);
		}
		daemon(0,1);
		dup2(lfd,1);
		dup2(lfd,2);
	}

	setpgrp(); /* needs for signalling */
	master = getpid();
	fpid = fopen(pidfile,"r");
	if ( fpid )
	{
		int	opid = -1;

		if ( fscanf(fpid,"%d",&opid) == 1 )
		{
			if ( !kill(opid,0) )
			{
				fprintf(stderr,"%s: another instance running, pid = %d\n",argv[0],opid);
				return 1;
			}
			else if (errno != ESRCH) { perror("kill()"); return 1; }
		}
		fclose(fpid);
	} else if (errno != ENOENT) { perror(pidfile); return 1; }

	fpid = fopen(pidfile, "w");
	if (!fpid) { perror(pidfile); return 1; }
	fprintf(fpid, "%d", getpid());
	fclose(fpid);

	glite_srvbones_set_param(GLITE_SBPARAM_SLAVES_COUNT,slaves);
	glite_srvbones_run(data_init,&stab,1 /* XXX: entries in stab */,debug);

	return 0;
}

static int data_init(void **data)
{
	*data = (void *) soap_new();

	printf("[%d] slave started\n",getpid());
	glite_jpps_srv_init(ctx);
	glite_jppsbe_init_slave(ctx);	/* XXX: global but slave's */
	//sleep(10);
	if (glite_jppsbe_purge_feeds(ctx) ||	/* XXX: is there a better place for the call? */
			glite_jppsbe_read_feeds(ctx)) fputs(glite_jp_error_chain(ctx),stderr);
	printf("[%d] slave init done\n",getpid());

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
        soap_set_omode(soap, SOAP_IO_BUFFER);   // set buffered response
                                                // buffer set to SOAP_BUFLEN (default = 8k)

	soap_set_namespaces(soap,jpps__namespaces);
	soap->user = (void *) ctx; /* XXX: one instance per slave */

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

				/* drop it too, it is recreated and reloads creds when necessary */
				if (ctx->other_soap) {
					soap_end(ctx->other_soap);
					soap_free(ctx->other_soap);
					ctx->other_soap = NULL;
				}
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
	glite_gsplugin_free_context(plugin_ctx); plugin_ctx = NULL;
	soap_end(soap);

	return ret;
}

static int request(int conn,struct timeval *to,void *data)
{
	struct soap		*soap = data;
	glite_jp_context_t	ctx = soap->user;

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
		|| jpps__serve_request(soap)
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
//printf("Ja cekam %d\n", getpid());
//sleep(10);
	if (glite_jp_run_deferred(ctx)) {
		char	*e;
		fprintf(stderr,"[%d] %s\n",getpid(),e = glite_jp_error_chain(ctx));
		free(e);
	}
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
	struct soap	*soap = (struct soap *) data;
	glite_jp_context_t	ctx = soap->user;

	soap_end(soap); // clean up everything and close socket
	if (ctx->other_soap) {
		soap_end(ctx->other_soap);
		soap_free(ctx->other_soap);
		ctx->other_soap = NULL;
	}

	return 0;
}

#define WSPACE "\t\n "

static int call_opts(glite_jp_context_t ctx,char *opt,char *name,int (*f)(glite_jp_context_t,int,char **))
{
	int	ac = 1,ret,my_optind; 
	char	**av = malloc(sizeof *av),*ap;

	*av = name;
	for (ap = strtok(opt,WSPACE); ap; ap = strtok(NULL,WSPACE)) {
		av = realloc(av,(ac+1) * sizeof *av);
		av[ac++] = ap;
	}

	my_optind = optind;
	optind = 0;
	ret = f(ctx,ac,av);
	optind = my_optind;
	free(av);
	return ret;
}


/* XXX: we don't use it */
SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };
