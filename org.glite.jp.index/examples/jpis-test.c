#include <stdio.h>
#include <sysexits.h>
#include <string.h>
#include <assert.h>

#include "glite/security/glite_gsplugin.h"

#include "jpis_H.h"
#include "jpis_.nsmap"

//#include "jptype_map.h"

#include "soap_version.h"
#if GSOAP_VERSION <= 20602
#define soap_call___jpsrv__UpdateJobs soap_call___ns1__UpdateJobs
#define soap_call___jpsrv__QueryJobs soap_call___ns1__QueryJobs
#endif


	
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


int main(int argc,char *argv[])
{
	char	*server = "http://localhost:8902";
	int	opt;
	struct soap	*soap = soap_new();


	soap_init(soap);
	soap_set_namespaces(soap, jpis__namespaces);

	soap_register_plugin(soap,glite_gsplugin);

	// test calls of server functions
	{
	// this call is issued by JPPS
		struct _jpelem__UpdateJobs 		in;
		struct _jpelem__UpdateJobsResponse 	out;

		memset(&in, 0, sizeof(in));
		memset(&out, 0, sizeof(out));
		check_fault(soap,
			soap_call___jpsrv__UpdateJobs(soap,server,"",&in,&out));
	}
	{
	// this call is issued by user
		struct _jpelem__QueryJobs		in;
		struct jptype__indexQuery 		*cond;
		struct jptype__indexQueryRecord 	*rec;
		struct _jpelem__QueryJobsResponse	out;

		
		in.__sizeconditions = 1;
		in.conditions = soap_malloc(soap,
			in.__sizeconditions * 
			sizeof(*(in.conditions)));
		
		cond = soap_malloc(soap, sizeof(*cond));
		memset(cond, 0, sizeof(*cond));
		cond->attr = soap_strdup(soap, "date");
		cond->origin = NULL;
		cond->__sizerecord = 1;
		cond->record = soap_malloc(soap, sizeof(*(cond->record)));

		rec = soap_malloc(soap, sizeof(*rec));
		memset(rec, 0, sizeof(*rec));
		rec->op = jptype__queryOp__GREATER;
		rec->value = soap_malloc(soap, sizeof(*(rec->value)));
		rec->value->string = soap_strdup(soap, "0");
		rec->value->blob = NULL;
		
		*(cond->record) = rec;
		*(in.conditions) = cond;

		in.__sizeattributes = 3;
		in.attributes = soap_malloc(soap,
			in.__sizeattributes *
			sizeof(*(in.attributes)));
		in.attributes[0] = soap_strdup(soap, "owner");
		in.attributes[1] = soap_strdup(soap, "date");
		in.attributes[2] = soap_strdup(soap, "location");

		memset(&out, 0, sizeof(out));

		check_fault(soap,
			soap_call___jpsrv__QueryJobs(soap, server, "",&in,&out));
	} 

	return 0;
}


/* XXX: we don't use it */
SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };
