#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include <glite/jp/types.h>
#include <glite/jp/context.h>

#include "jpis_H.h"
#include "jpis_.nsmap"
#include "soap_version.h"
#include "db_ops.h"
#include "ws_typeref.h"



/*------------------*/
/* Helper functions */
/*------------------*/


static struct jptype__genericFault *jp2s_error(struct soap *soap,
		const glite_jp_error_t *err)
{
	struct jptype__genericFault *ret = NULL;
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
	struct SOAP_ENV__Detail	*detail = soap_malloc(soap,sizeof *detail);
	struct _genericFault *f = soap_malloc(soap,sizeof *f);


	f->jpelem__genericFault = jp2s_error(soap,ctx->error);

	detail->__type = SOAP_TYPE__genericFault;
#if GSOAP_VERSION >= 20700
	detail->fault = f;
#else
	detail->value = f;
#endif
	detail->__any = NULL;

	soap_receiver_fault(soap,"Oh, shit!",NULL);
	if (soap->version == 2) soap->fault->SOAP_ENV__Detail = detail;
	else soap->fault->detail = detail;
}





/*-----------------------------------------*/
/* IS WSDL server function implementations */
/*-----------------------------------------*/


#define CONTEXT_FROM_SOAP(soap,ctx) glite_jpis_context_t	ctx = (glite_jpis_context_t) ((soap)->user)


static int updateJob(glite_jpis_context_t isctx, struct jptype__jobRecord *jobAttrs) {
	glite_jp_attrval_t av;
	struct jptype__attrValue *attr;
	int ret, iattrs;

	printf("%s: jobid='%s', attrs=%d\n", __FUNCTION__, jobAttrs->jobid, jobAttrs->__sizeattributes);

	if (jobAttrs->remove) assert(*(jobAttrs->remove) == 0);

	for (iattrs = 0; iattrs < jobAttrs->__sizeattributes; iattrs++) {
		attr = jobAttrs->attributes[iattrs];
		glite_jpis_SoapToAttrVal(&av, attr);
		if ((ret = glite_jpis_insertAttrVal(isctx, jobAttrs->jobid, &av)) != 0) return ret;
	}

	return 0;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__UpdateJobs(
	struct soap *soap,
	struct _jpelem__UpdateJobs *jpelem__UpdateJobs,
	struct _jpelem__UpdateJobsResponse *jpelem__UpdateJobsResponse)
{
	int ret, ijobs;
	const char *feedid;
	int status, done;
	CONTEXT_FROM_SOAP(soap, isctx);
	glite_jp_context_t jpctx = isctx->jpctx;

	// XXX: test client in examples/jpis-test
	//      sends to this function some data for testing
	puts(__FUNCTION__);

	// get info about the feed
	feedid = jpelem__UpdateJobs->feedId;
	memset(isctx->param_feedid, 0, sizeof(isctx->param_feedid));
	strncpy(isctx->param_feedid, feedid, sizeof(isctx->param_feedid) - 1);
	if ((ret = glite_jp_db_execute(isctx->select_info_feed_stmt)) != 1) {
		fprintf(stderr, "can't get info about '%s', returned %d records: %s (%s)\n", feedid, ret, jpctx->error->desc, jpctx->error->source);
		return SOAP_FAULT;
	}
	// update status, if needed (only oring)
	status = isctx->param_state;
	done = jpelem__UpdateJobs->feedDone ? GLITE_JP_IS_STATE_DONE : 0;
	if ((done != (status & GLITE_JP_IS_STATE_DONE)) && done) {
		isctx->param_state |= done;
		if ((ret = glite_jp_db_execute(isctx->update_state_feed_stmt)) != 1) {
			fprintf(stderr, "can't update state of '%s', returned %d records: %s (%s)\n", feedid, ret, jpctx->error->desc, jpctx->error->source);
			return SOAP_FAULT;
		}
	}

	// insert all attributes
	for (ijobs = 0; ijobs < jpelem__UpdateJobs->__sizejobAttributes; ijobs++) {
		if (updateJob(isctx, jpelem__UpdateJobs->jobAttributes[ijobs]) != 0) return SOAP_FAULT;
	}

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__QueryJobs(
	struct soap *soap,
	struct _jpelem__QueryJobs *jpelem__QueryJobs,
	struct _jpelem__QueryJobsResponse *jpelem__QueryJobsResponse)
{
	puts(__FUNCTION__);
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__AddFeed(
        struct soap *soap,
        struct _jpelem__AddFeed *in,
        struct _jpelem__AddFeedResponse *out)
{
        // XXX: test client in examples/jpis-test
        //      sends to this function some data for testing
        puts(__FUNCTION__);
        return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__GetFeedIDs(
        struct soap *soap,
        struct _jpelem__GetFeedIDs *in,
        struct _jpelem__GetFeedIDsResponse *out)
{
        // XXX: test client in examples/jpis-test
        //      sends to this function some data for testing
        puts(__FUNCTION__);
        return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__DeleteFeed(
        struct soap *soap,
        struct _jpelem__DeleteFeed *in,
        struct _jpelem__DeleteFeedResponse *out)
{
        // XXX: test client in examples/jpis-test
        //      sends to this function some data for testing
        puts(__FUNCTION__);
        return SOAP_OK;
}

