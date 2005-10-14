#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "glite/jp/types.h"
#include "glite/security/glite_gsplugin.h"

#include "feed.h"
#include "is_client.h"

#include "jpis_ClientLib.c"
#include "jpis_.nsmap"

#include "soap_util.c"

extern char *server_key, *server_cert;	/* XXX */

static int check_other_soap(glite_jp_context_t ctx)
{
	glite_gsplugin_Context	plugin_ctx;

	if (!ctx->other_soap) {
		glite_gsplugin_init_context(&plugin_ctx);
		if (server_key) plugin_ctx->key_filename = strdup(server_key);
		if (server_cert) plugin_ctx->cert_filename = strdup(server_cert);

		ctx->other_soap = soap_new();
		soap_init(ctx->other_soap);
		soap_set_namespaces(ctx->other_soap,jpis__namespaces);
		soap_register_plugin_arg(ctx->other_soap,glite_gsplugin,plugin_ctx);
		ctx->other_soap->user = ctx;
	}
	return 0;
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
	struct _jpelem__UpdateJobs	in;
	struct _jpelem__UpdateJobsResponse	out;
	struct jptype__jobRecord	jr, *jrp = &jr;
	int	i;
	enum xsd__boolean	false = false_;
	glite_jp_error_t	err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);

	/* TODO: call JP Index server via interlogger */

	printf("feed to %s, job %s\n",destination,job);

	check_other_soap(ctx);

	in.feedId = (char *) feed; /* XXX: const */
	in.feedDone = done;
	in.__sizejobAttributes = 1;
	in.jobAttributes = &jrp;

	for (i=0; attrs[i].name; i++);
	jr.jobid = (char *) job; /* XXX: const */
	jr.owner = owner;

	jr.__sizeattributes = jp2s_attrValues(ctx->other_soap,
			(glite_jp_attrval_t *) attrs, /* XXX: const */
			&jr.attributes,0);

	jr.remove = &false;
	jr.__sizeprimaryStorage = 1;
	jr.primaryStorage = &ctx->myURL;


	if (soap_call___jpsrv__UpdateJobs(ctx->other_soap,destination,"",
		&in,&out
	)) {
		char	buf[1000];
		err.code = EIO;
		err.source = __FUNCTION__;
		err.desc = buf;
		snprintf(buf,sizeof buf,"%s %s\n",
				ctx->other_soap->fault->faultcode,
				ctx->other_soap->fault->faultstring);
		buf[999] = 0;
		glite_jp_stack_error(ctx,&err);
	}

	attrValues_free(ctx->other_soap,jr.attributes,jr.__sizeattributes);

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
	int	i,j;

	struct _jpelem__UpdateJobs	in;
	struct _jpelem__UpdateJobsResponse	out;
	struct jptype__jobRecord	*jr;
	enum xsd__boolean	false = false_;
	glite_jp_error_t	err;

	printf("multi_feed: %s\n",destination);

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);

	check_other_soap(ctx);

	in.feedId = (char *) feed; /* XXX: const */
	in.feedDone = done;
	in.__sizejobAttributes = njobs;
	in.jobAttributes = malloc(njobs * sizeof *in.jobAttributes);

	for (i=0; i<njobs; i++) {
		puts(jobs[i]);
		for (j=0; attrs[i][j].name; j++)
			printf("%s = %s\n",attrs[i][j].name,attrs[i][j].value);
		putchar(10);

		in.jobAttributes[i] = jr = malloc(sizeof *jr);
		jr->jobid = jobs[i];
		jr->owner = owners[i];

		jr->__sizeattributes = jp2s_attrValues(ctx->other_soap,
			attrs[i],
			&jr->attributes,0);

		jr->remove = &false;
		jr->__sizeprimaryStorage = 1;
		jr->primaryStorage = &ctx->myURL;
	}

	check_fault(ctx,ctx->other_soap,
		soap_call___jpsrv__UpdateJobs(ctx->other_soap,destination,"", &in,&out));
	for (i=0; i<njobs; i++) {
		jr = in.jobAttributes[i];
		attrValues_free(ctx->other_soap,jr->attributes,jr->__sizeattributes);
		free(jr);
	}
	free(in.jobAttributes);

	return err.code;
}

