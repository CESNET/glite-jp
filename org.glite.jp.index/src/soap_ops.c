#include <stdio.h>
#include <fcntl.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "jpis_H.h"
#include "jpis_.nsmap"
// XXX: need solve 2 WSDLs problem :(
//#include "jpps_H.h"
//#include "jpps_.nsmap"
#include "soap_version.h"





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
	char	*et;
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


#define CONTEXT_FROM_SOAP(soap,ctx) glite_jp_context_t	ctx = (glite_jp_context_t) ((soap)->user)

SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__UpdateJobs(
	struct soap *soap,
	struct _jpelem__UpdateJobs *jpelem__UpdateJobs,
	struct _jpelem__UpdateJobsResponse *jpelem__UpdateJobsResponse)
{
	puts(__FUNCTION__);
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





/*----------------------*/
/* PS WSDL client calls */
/*----------------------*/

/*
// XXX: need solve 2 WSDLs problem :(

// call PS FeedIndex for a given destination
void MyFeedIndex(glite_jp_is_conf *conf, char *dest)
{
	struct _jpelem__FeedIndex		in;
	struct jpsrv__FeedIndexResponse 	out;
	struct jptype__PrimaryQuery     	query;
	struct jptype__stringOrBlob		value;
//	struct xsd__base64Binary		blob;
	int 					i;


	memset(&in, 0, sizeof(in));

	for (i=0; conf->attrs[i]; i++) ;
	in.__sizeattributes = i;
	in.attributes = conf->attrs;

	// XXX: we need C -> WSDL conversion function !
	query.attr = conf->query[0][0].attr;
	query.op = conf->query[0][0].op; 	// XXX: nasty, needs conversion
	query.origin = jptype__attrOrig__USER;
	value.string = conf->query[0][0].value; // XXX: hope string
//	memset(&blob, 0, sizeof(blob));
//	value.blob = &blob;
	value.blob = NULL;
	query.value = &value;
	query.value2 = NULL;

	in.__sizeconditions = 1
	in.conditions = malloc(sizeof(*in.conditions));
	in.conditions[0] = &query;	// XXX: supp. only one dimensional queries ! (no ORs)
					// for 2D queries one more _sizeconditions needed IMO

	in.history = conf->history;
	in.continuous = conf->continuous;

	if (!check_fault(soap,soap_call_jpsrv___FeedIndex(soap,server,"",
			dest, &in, &out)))
	{
		printf("FeedId: %s\nExpires: %s\n",out.feedId,ctime(&out.expires));
	}

}
*/
