#include <stdio.h>
#include <fcntl.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "jpis_H.h"
#include "JobProvenanceIS.nsmap"

static struct jptype__GenericJPFaultType *jp2s_error(struct soap *soap,
		const glite_jp_error_t *err)
{
	struct jptype__GenericJPFaultType *ret = NULL;
	if (err) {
		ret = soap_malloc(soap,sizeof *ret);
		memset(ret,0,sizeof *ret);
		ret->code = err->code;
		ret->source = soap_strdup(soap,err->source);
		ret->text = soap_strdup(soap,strerror(err->code));
		ret->description = soap_strdup(soap,err->desc);
		ret->reason = jp2s_error(soap,err->reason);
	}
	return ret;
}

static void err2fault(const glite_jp_context_t ctx,struct soap *soap)
{
	char	*et;
	struct SOAP_ENV__Detail	*detail = soap_malloc(soap,sizeof *detail);
	struct _GenericJPFault *f = soap_malloc(soap,sizeof *f);


	f->jptype__GenericJPFault = jp2s_error(soap,ctx->error);

	detail->__type = SOAP_TYPE__GenericJPFault;
	detail->value = f;
	detail->__any = NULL;

	soap_receiver_fault(soap,"Oh, shit!",NULL);
	if (soap->version == 2) soap->fault->SOAP_ENV__Detail = detail;
	else soap->fault->detail = detail;
}

static void s2jp_tag(const struct jptype__TagValue *stag,glite_jp_tagval_t *jptag)
{
	memset(jptag,0,sizeof *jptag);
	jptag->name = strdup(stag->name);
	jptag->sequence = stag->sequence ? *stag->sequence : 0;
	jptag->timestamp = stag->timestamp ? *stag->timestamp : 0;
	if (stag->stringValue) jptag->value = strdup(stag->stringValue);
	else if (stag->blobValue) {
		jptag->binary = 1;
		jptag->size = stag->blobValue->__size;
		jptag->value = (char *) stag->blobValue->__ptr;
	}
}

#define CONTEXT_FROM_SOAP(soap,ctx) glite_jp_context_t	ctx = (glite_jp_context_t) ((soap)->user)

SOAP_FMAC5 int SOAP_FMAC6 jpsrv__UpdateJobs(
	struct soap *soap,
	char *feed_id,
	struct jptype__UpdateJobsData *jobs,
	enum xsd__boolean done
)
{
	printf("%s items %d jobid %s\n",__FUNCTION__,jobs->__sizejob,
			jobs->job[0]->jobid);
	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 jpsrv__QueryJobs(
	struct soap *soap,
	struct jptype__IndexQuery *query,
	struct jpsrv__QueryJobsResponse *resp
)
{
	puts(__FUNCTION__);
	return SOAP_OK;
}

