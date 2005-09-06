#include <stdio.h>
#include <fcntl.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "jpps_H.h"
#include "jpps_.nsmap"
#include "soap_version.h"

#include "conf.h"
#include "db_ops.h"

#include "stdsoap2.h"

extern int glite_jpis_QueryCondToSoap(struct soap *soap, glite_jp_query_rec_t *in, struct jptype__primaryQuery **out);


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



/*----------------------*/
/* PS WSDL client calls */
/*----------------------*/


// call PS FeedIndex for a given destination
void MyFeedIndex(glite_jp_is_conf *conf, char *dest)
{
	struct _jpelem__FeedIndex		in;
	struct _jpelem__FeedIndexResponse 	out;
	struct jptype__primaryQuery     	query;
	struct jptype__stringOrBlob		value;
//	struct xsd__base64Binary		blob;
	int 					i;
	struct soap             		*soap = soap_new();


printf("MyFeedIndex for %s called\n", dest);

	soap_init(soap);
        soap_set_namespaces(soap,jpps__namespaces);

	memset(&in, 0, sizeof(in));

	for (i=0; conf->attrs[i]; i++) ;
	in.__sizeattributes = i;
	in.attributes = conf->attrs;

	for (in.__sizeconditions=0; conf->query[in.__sizeconditions]; in.__sizeconditions++);
	in.conditions = malloc(in.__sizeconditions * sizeof(*in.conditions));

	for (i=0; conf->query[i]; i++) {
		if (glite_jpis_QueryCondToSoap(soap, conf->query[i], &(in.conditions[i])) != SOAP_OK) {
			printf("MyFeedIndex() - error during conds conversion\n");
			goto err;
		}
	}

	in.history = conf->history;
	in.continuous = conf->continuous;

	//if (!check_fault(soap,soap_call_jpsrv___FeedIndex(soap,dest,"",
	if (soap_call___jpsrv__FeedIndex(soap,dest,"", &in, &out)) {
		printf("soap_call___jpsrv__FeedIndex() returned error\n");
		glite_jpis_unlockFeed(dest);
		goto err;
	}
	else {
		printf("FeedId: %s\nExpires: %s\n",out.feedId,ctime(&out.feedExpires));
		glite_jpis_feedInit(dest, out.feedId, out.feedExpires);
		glite_jpis_unlockFeed(dest);
	}
	
err:
	soap_end(soap);
}

