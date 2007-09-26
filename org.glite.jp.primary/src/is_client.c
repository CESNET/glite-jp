#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

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

static int check_other_soap(glite_jp_context_t ctx)
{
	glite_gsplugin_Context	plugin_ctx;
	int			ret = 0;

	if (!ctx->other_soap) {
		glite_gsplugin_init_context(&plugin_ctx);
		if (server_key || server_cert) {
			edg_wll_GssCred cred;

			ret = edg_wll_gss_acquire_cred_gsi(server_cert, server_key, &cred, NULL, NULL);
			glite_gsplugin_set_credential(plugin_ctx, cred);
		}

		ctx->other_soap = soap_new();
		soap_init(ctx->other_soap);
		soap_set_namespaces(ctx->other_soap,jpis__namespaces);
		soap_set_omode(ctx->other_soap, SOAP_IO_BUFFER);	// set buffered response
                                          		     		// buffer set to SOAP_BUFLEN (default = 8k)
		soap_register_plugin_arg(ctx->other_soap,glite_gsplugin,plugin_ctx);
		ctx->other_soap->user = ctx;
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

		jr->__sizeattributes = jp2s_attrValues(ctx->other_soap,
			attrs[i],
			&jr->attributes,0);

		jr->remove = &false;
		jr->__sizeprimaryStorage = 1;
		jr->primaryStorage = &ctx->myURL;
	}

	SWITCH_SOAP_CTX
	check_fault(ctx,ctx->other_soap,
		soap_call___jpsrv__UpdateJobs(ctx->other_soap,destination,"", &in,&out));
	RESTORE_SOAP_CTX
	for (i=0; i<njobs; i++) {
		jr = GLITE_SECURITY_GSOAP_LIST_GET(in.jobAttributes, i);
		attrValues_free(ctx->other_soap,jr->attributes,jr->__sizeattributes);
	}
	GLITE_SECURITY_GSOAP_LIST_DESTROY(ctx->other_soap, &in, jobAttributes);

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
