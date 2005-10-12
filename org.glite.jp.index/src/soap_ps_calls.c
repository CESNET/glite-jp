#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "jpps_H.h"
#include "jpps_.nsmap"
#include "soap_version.h"

#include "conf.h"
#include "db_ops.h"
#include "ws_ps_typeref.h"
#include "context.h"

#include "stdsoap2.h"


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


static int check_fault(struct soap *soap,int err) {
	struct SOAP_ENV__Detail *detail;
	struct jptype__genericFault	*f;
	char	*reason,indent[200] = "  ";

	switch(err) {
		case SOAP_OK: puts("OK");
			      break;
		case SOAP_FAULT:
		case SOAP_SVR_FAULT:
			if (soap->version == 2) {
				detail = soap->fault->SOAP_ENV__Detail;
				reason = soap->fault->SOAP_ENV__Reason;
			}
			else {
				detail = soap->fault->detail;
				reason = soap->fault->faultstring;
			}
			fputs(reason,stderr);
			putc('\n',stderr);
			assert(detail->__type == SOAP_TYPE__genericFault);
#if GSOAP_VERSION >=20700
			f = ((struct _genericFault *) detail->fault)
#else
			f = ((struct _genericFault *) detail->value)
#endif
				-> jpelem__genericFault;

			while (f) {
				fprintf(stderr,"%s%s: %s (%s)\n",indent,
						f->source,f->text,f->description);
				f = f->reason;
				strcat(indent,"  ");
			}
			return -1;

		default: soap_print_fault(soap,stderr);
			 return -1;
	}
	return 0;
}


/*----------------------*/
/* PS WSDL client calls */
/*----------------------*/

static int find_dest_index(glite_jp_is_conf *conf, char *dest)
{
	int i;

	for (i=0; conf->feeds[i]; i++)
		if (!strcmp(dest, conf->feeds[i]->PS_URL)) return(i);

	return -1;
}


// call PS FeedIndex for a given destination
int MyFeedIndex(glite_jpis_context_t ctx, glite_jp_is_conf *conf, long int uniqueid, char *dest)
{
	struct _jpelem__FeedIndex		in;
	struct _jpelem__FeedIndexResponse 	out;
//	struct jptype__primaryQuery     	query;
//	struct jptype__stringOrBlob		value;
//	struct xsd__base64Binary		blob;
	int 					i, dest_index;
	struct soap             		*soap = soap_new();
	glite_jp_error_t err;
	char *src;

printf("MyFeedIndex for %s called\n", dest);

	soap_init(soap);
        soap_set_namespaces(soap,jpps__namespaces);

	memset(&in, 0, sizeof(in));
	memset(&err, 0, sizeof(err));

	for (i=0; conf->attrs[i]; i++) ;
	in.__sizeattributes = i;
	in.attributes = conf->attrs;

	if ((dest_index = find_dest_index(conf, dest)) < 0) goto err;

	for (i=0; conf->feeds[dest_index]->query[i]; i++);
	in.__sizeconditions = i;
	in.conditions = soap_malloc(soap, in.__sizeconditions * sizeof(*in.conditions));

	for (i=0; conf->feeds[dest_index]->query[i]; i++) {
		if (glite_jpis_QueryCondToSoap(soap, conf->feeds[dest_index]->query[i], 
				&(in.conditions[i])) != SOAP_OK) {
			err.code = EINVAL;
			err.desc = "error during conds conversion";
			asprintf(&src, "%s/%s():%d", __FILE__, __FUNCTION__, __LINE__);
			printf("%s\n", src);
			goto err;
		}
	}

	in.history = conf->feeds[dest_index]->history;
	in.continuous = conf->feeds[dest_index]->continuous;

	if (check_fault(soap,soap_call___jpsrv__FeedIndex(soap,dest,"", &in, &out)) != 0) {
		printf("\n");
		glite_jpis_unlockFeed(ctx, uniqueid);
		err.code = EIO;
		err.desc = "soap_call___jpsrv__FeedIndex() returned error";
		asprintf(&src, "%s/%s():%d", __FILE__, __FUNCTION__, __LINE__);
		printf("%s\n", err.desc);
		goto err;
	}
	else {
		printf("FeedId: %s\nExpires: %s\n",out.feedId,ctime(&out.feedExpires));
		glite_jpis_initFeed(ctx, uniqueid, out.feedId, out.feedExpires);
		glite_jpis_unlockFeed(ctx, uniqueid);
	}

	return 0;
err:
	err.source = src;
	glite_jp_stack_error(ctx->jpctx, &err);
	free(src);
	soap_end(soap);
	return err.code;
}

