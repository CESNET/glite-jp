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
		struct _jpelem__QueryJobs in;
		struct _jpelem__QueryJobsResponse out;

		memset(&in, 0, sizeof(in));
		memset(&out, 0, sizeof(out));
		check_fault(soap,
			soap_call___jpsrv__UpdateJobs(soap,server,"",&in,&out));
	}
	{
		struct _jpelem__QueryJobs		in;
		struct _jpelem__QueryJobsResponse	out;

		memset(&in, 0, sizeof(in));
		memset(&out, 0, sizeof(out));
		check_fault(soap,
			soap_call___jpsrv__QueryJobs(soap, server, "",&in,&out));
	} 

	return 0;
}


/* XXX: we don't use it */
SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };
