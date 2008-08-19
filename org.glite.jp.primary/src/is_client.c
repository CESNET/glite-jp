#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#undef SOAP_FMAC1
#define SOAP_FMAC1 static

#include <stdsoap2.h>

#include "glite/jp/types.h"
#include "soap_version.h"
#include "glite/security/glite_gsplugin.h"
#include "glite/security/glite_gscompat.h"

#include "feed.h"
#include "is_client.h"

/* same as ClientLib.c, without WITH_NOGLOBAL */
#define SOAP_FMAC3 static
#include "jpis_C.c"
#include "jpis_Client.c"

#include "jpis_.nsmap"

#include "soap_env_ctx.h"
#include "soap_env_ctx.c"

#include "glite/jp/ws_fault.c"
#include "soap_util.c"


#define MAX_RETRY	10
#define RETRY_SLEEP	2

extern char *server_key, *server_cert;	/* XXX */
extern char file_prefix[];
extern char *il_sock;

struct client_data {
	char *host;
	int port;
	long offset;
};

/*
static int fconnect(struct soap *soap, const char *endpoint, const char *host, int port){
	int s, len;
	const char *dest = "/tmp/il_sock";
	struct sockaddr_un remote;

        if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return SOAP_ERR; 

        remote.sun_family = AF_UNIX;
        strcpy(remote.sun_path, dest);
        len = strlen(remote.sun_path) + sizeof(remote.sun_family);
        if (connect(s, (struct sockaddr *)&remote, len) < 0)
		return SOAP_ERR; 

	soap->socket = s;

	return SOAP_OK;
}
*/

void notify_il_sock(struct soap *soap) {
	struct client_data *data = soap->user;
	int s, len;
	struct sockaddr_un remote;
	char *buf;

	if(il_sock == NULL) return;
	if(data == NULL) return;

        if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return; 

        remote.sun_family = AF_UNIX;
        strcpy(remote.sun_path, il_sock);
        len = strlen(remote.sun_path) + sizeof(remote.sun_family);
        if (connect(s, (struct sockaddr *)&remote, len) < 0) {
		close(s);
		return; 
	}

	asprintf(&buf, "POST / HTTP/1.1\r\nHost: %s:%d\r\nContent-Length: 1\r\n\r\n",
		 data->host, data->port);
	if(buf) {
		/* fire and forget */
		send(s, buf, strlen(buf) + 1, MSG_NOSIGNAL | MSG_DONTWAIT);
		free(buf);
	}
	free(remote.sun_path);
	close(s);
	return;
}


int myopen(struct soap *soap, const char *endpoint, const char *host, int port)
{
	char filename[PATH_MAX];
	FILE *outfile;
	int i, filedesc;
	struct client_data *data;

	data = malloc(sizeof(*data));
	if(data == NULL)
		return ENOMEM;

	/* XXX: should it be strdup'ed? */
	data->host = host;
	data->port = port;

	snprintf(filename, PATH_MAX-1, "%s.%s:%d", file_prefix, host, port);
	filename[PATH_MAX - 1] = 0;	

try_again:
	if((outfile = fopen(filename, "a")) == NULL) {
		goto cleanup;
	}
	if((filedesc = fileno(outfile)) < 0) {
		goto cleanup;
	}

	for(i = 0; i < 5; i++) {
		struct flock filelock;
		int filelock_status;
		struct stat statbuf;

		filelock.l_type = F_WRLCK;
		filelock.l_whence = SEEK_SET;
		filelock.l_start = 0;
		filelock.l_len = 0;

		if((filelock_status=fcntl(filedesc, F_SETLK, &filelock)) < 0) {
			switch(errno) {
			case EAGAIN:
			case EACCES:
			case EINTR:
				if((i+1) < 5) sleep(1);
				break;
			default:
				goto cleanup;
			}
		} else {
			if(stat(filename, &statbuf)) {
				if(errno == ENOENT) {
					fclose(outfile);
					goto try_again;
				} else {
					goto cleanup;
				}
			} else {
				/* success */
				break;
			}
		}
	}

	if(i == 5) {
		errno = ETIMEDOUT;
		goto cleanup;
	}

	data->offset = ftell(outfile);
	soap->user = data;
	soap->sendfd = filedesc;
	return filedesc;

cleanup:
	filedesc = errno;
	if(outfile) fclose(outfile);
	return filedesc;
}


int myclose(struct soap *soap)
{
	if(soap->sendfd > 2) {
		write(soap->sendfd, "\n", 1);
		close(soap->sendfd);
		soap->sendfd = -1;
		/* send message to IL on socket */
		notify_il_sock(soap);
		if(soap->user) {
			free((struct client_data*)soap->user);
			soap->user = NULL;
		}
	}
	return SOAP_OK;
}

int mysend(struct soap *soap, const char *s, size_t n)
{
	int ret;

	ret = write(soap->sendfd, s, n);
	if(ret < 0) 
		return ret;
	return SOAP_OK;
}

size_t myrecv(struct soap *soap, char *s, size_t n)
{
	const char response[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<SOAP-ENV:Envelope"
		" xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\""
		" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\""
		" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
		" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
		" xmlns:ns3=\"http://glite.org/wsdl/types/jp\""
		" xmlns:ns1=\"http://glite.org/wsdl/services/jp\""
		" xmlns:ns2=\"http://glite.org/wsdl/elements/jp\">"
		" <SOAP-ENV:Body>"
		"  <ns2:UpdateJobsResponse>"
		"  </ns2:UpdateJobsResponse>"
		" </SOAP-ENV:Body>"
		"</SOAP-ENV:Envelope>";

	int len;

	if(soap->user == NULL)
		soap->user = response;

	len = sizeof(response) - ((char*)soap->user - response);
	if(n < len) len = n;
	strncpy(s, (char*)soap->user, len);
	soap->user += len;
	return len;
}

static int check_other_soap(glite_jp_context_t ctx)
{
	glite_gsplugin_Context	plugin_ctx;
	int			ret = 0;

	if (!ctx->other_soap) {
		glite_gsplugin_init_context(&plugin_ctx);
		if (server_key || server_cert) {
			edg_wll_GssCred cred;

			ret = edg_wll_gss_acquire_cred_gsi(server_cert, server_key, &cred, NULL);
			glite_gsplugin_set_credential(plugin_ctx, cred);
		}

		ctx->other_soap = soap_new();
		soap_init(ctx->other_soap);
		soap_set_namespaces(ctx->other_soap,jpis__namespaces);
		soap_set_omode(ctx->other_soap, SOAP_IO_BUFFER);	// set buffered response
                                          		     		// buffer set to SOAP_BUFLEN (default = 8k)
		if(il_sock == NULL) {
			/* send to JPIS directly */
			soap_register_plugin_arg(ctx->other_soap,glite_gsplugin,plugin_ctx);
			ctx->other_soap->user = ctx;
		} else {
			/* this makes the SOAP client send all traffic through local socket to IL */
			// ctx->other_soap->fconnect = fconnect;
			ctx->other_soap->fopen = myopen;
			ctx->other_soap->fclose = myclose;
			ctx->other_soap->fsend = mysend;
		}
	}
	return ret;
}

static check_fault(glite_jp_context_t ctx,struct soap *soap,int ec)
{
	glite_jp_error_t	err;
	char	buf[1000] = "unknown fault";

	switch (ec) {
		case SOAP_OK: return 0;
		default: 
			err.code = EIO;
			err.source = __FUNCTION__;
			err.desc = buf;
			if (soap->fault) snprintf(buf,sizeof buf,"%s %s\n",
				soap->fault->faultcode,
				soap->fault->faultstring);
			buf[999] = 0;
			glite_jp_stack_error(ctx,&err);
			return err.code;
	}
}

static struct _glite_jp_soap_env_ctx_t *keep_soap_env_ctx;

#define SWITCH_SOAP_CTX \
{ \
	keep_soap_env_ctx = glite_jp_soap_env_ctx; \
	glite_jp_soap_env_ctx = &my_soap_env_ctx; \
} \

#define RESTORE_SOAP_CTX \
{ \
	glite_jp_soap_env_ctx = keep_soap_env_ctx; \
} \

static int glite_jpps_single_feed_wrapped(
		glite_jp_context_t ctx,
		const char *feed,
		int	done,
		const char *destination,
		const char *job,
		const char *owner,
		glite_jp_attrval_t const *attrs
)
{
	struct _jpelem__UpdateJobs	in;
	struct _jpelem__UpdateJobsResponse	out;
	struct jptype__jobRecord	jr, *jrp = &jr;
	int	i;
	enum xsd__boolean	false = GLITE_SECURITY_GSOAP_FALSE;
	glite_jp_error_t	err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);

	/* TODO: call JP Index server via interlogger */

	printf("feed %s to %s, job %s\n",feed,destination,job);

	check_other_soap(ctx);

	in.feedId = (char *) feed; /* XXX: const */
	in.feedDone = done;
	in.__sizejobAttributes = 1;
#warning FIXME for valtri
	in.jobAttributes = &jrp;

	for (i=0; attrs[i].name; i++);
	jr.jobid = soap_strdup(ctx->other_soap, job);
	jr.owner = soap_strdup(ctx->other_soap, owner);

	jr.__sizeattributes = jp2s_attrValues(ctx->other_soap,
			(glite_jp_attrval_t *) attrs, /* XXX: const */
			&jr.attributes,0);

	jr.remove = &false;
	jr.__sizeprimaryStorage = 1;
	jr.primaryStorage = &ctx->myURL;


	SWITCH_SOAP_CTX
	if (soap_call___jpsrv__UpdateJobs(ctx->other_soap,destination,"",
		&in,&out
	)) {
		char	buf[1000];
		err.code = EIO;
		err.source = __FUNCTION__;
		err.desc = buf;
		memset(buf, 0, sizeof(buf));
		if (ctx->other_soap->fault) {
			snprintf(buf,sizeof buf,"%s %s\n",
				ctx->other_soap->fault->faultcode,
				ctx->other_soap->fault->faultstring);
		}
		else {
			sprintf(buf,"No detailed error description (JP IS not running?)\n");
		}
		buf[999] = 0;
		glite_jp_stack_error(ctx,&err);
	}
	RESTORE_SOAP_CTX

	soap_dealloc(ctx->other_soap, jr.jobid);
	soap_dealloc(ctx->other_soap, jr.owner);
	attrValues_free(ctx->other_soap,jr.attributes,jr.__sizeattributes);

	return err.code;
}

int glite_jpps_single_feed(
		glite_jp_context_t ctx,
		const char *feed,
		int	done,
		const char *destination,
		const char *job,
		const char *owner,
		glite_jp_attrval_t const *attrs
)
{
	int	retry,ret;
	assert(owner);
	for (retry = 0; retry < MAX_RETRY; retry++) {
		if ((ret = glite_jpps_single_feed_wrapped(ctx,feed,done,destination,job,owner,attrs)) == 0) break;
		sleep(RETRY_SLEEP);
	}
	return ret;
}


static int glite_jpps_multi_feed_wrapped(
		glite_jp_context_t ctx,
		const char *feed,
		int	done,
		int njobs,
		const char *destination,
		char **jobs,
		char **owners,
		glite_jp_attrval_t **attrs)
{
	int	i,j;

	struct _jpelem__UpdateJobs	in;
	struct _jpelem__UpdateJobsResponse	out;
	struct jptype__jobRecord	*jr;
	enum xsd__boolean	false = GLITE_SECURITY_GSOAP_FALSE;
	glite_jp_error_t	err;

	printf("multi_feed %s to %s\n",feed,destination);

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);

	check_other_soap(ctx);

	in.feedId = (char *) feed; /* XXX: const */
	in.feedDone = done;
	GLITE_SECURITY_GSOAP_LIST_CREATE(ctx->other_soap, &in, jobAttributes, struct jptype__jobRecord, njobs);

	for (i=0; i<njobs; i++) {
		puts(jobs[i]);
		for (j=0; attrs[i][j].name; j++)
			printf("%s = %s\n",attrs[i][j].name,attrs[i][j].value);
		putchar(10);

		jr = GLITE_SECURITY_GSOAP_LIST_GET(in.jobAttributes, i);
		jr->jobid = jobs[i];
		jr->owner = owners[i];

		assert(jr->owner);

		jr->__sizeattributes = jp2s_attrValues(ctx->other_soap,
			attrs[i],
			&jr->attributes,0);

		jr->remove = &false;
		jr->__sizeprimaryStorage = 1;
		jr->primaryStorage = &ctx->myURL;
	}

	//#ifndef JP_PERF
	SWITCH_SOAP_CTX
	check_fault(ctx,ctx->other_soap,
		soap_call___jpsrv__UpdateJobs(ctx->other_soap,destination,"", &in,&out));
	RESTORE_SOAP_CTX
	for (i=0; i<njobs; i++) {
		jr = GLITE_SECURITY_GSOAP_LIST_GET(in.jobAttributes, i);
		attrValues_free(ctx->other_soap,jr->attributes,jr->__sizeattributes);
	}
	GLITE_SECURITY_GSOAP_LIST_DESTROY(ctx->other_soap, &in, jobAttributes);
	//#endif

	return err.code;
}


int glite_jpps_multi_feed(
		glite_jp_context_t ctx,
		const char *feed,
		int	done,
		int njobs,
		const char *destination,
		char **jobs,
		char **owners,
		glite_jp_attrval_t **attrs)
{
	int	retry,ret;
	for (retry = 0; retry < MAX_RETRY; retry++) {
		if ((ret = glite_jpps_multi_feed_wrapped(ctx,feed,done,njobs,destination,jobs,owners,attrs)) == 0) break;
		sleep(RETRY_SLEEP);
	}
	return ret;
}
