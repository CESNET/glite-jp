#include <stdio.h>
#include <sysexits.h>
#include <string.h>
#include <assert.h>

#include "jpps_H.h"
#include "jpps_.nsmap"

#include "jptype_map.h"

static void usage(const char *me)
{
	fprintf(stderr,"%s: [-s server-url] operation args \n\n"
			"	operations are:\n"
			"		RegisterJob jobid\n"
			"		StartUpload jobid class(0,1,2) commit_before mimetype\n"
			"		CommitUpload\n"
			"		RecordTag\n"
			"		GetJob\n"
			"		FeedIndex destination query_number history continuous\n"
		,me);

	exit (EX_USAGE);
}
	
static int check_fault(struct soap *soap,int err) {
	struct SOAP_ENV__Detail *detail;
	struct jptype__GenericJPFaultType	*f;
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
			assert(detail->__type == SOAP_TYPE__GenericJPFault);
#if GSOAP_VERSION >=20700
			f = ((struct _GenericJPFault *) detail->fault)
#else
			f = ((struct _GenericJPFault *) detail->value)
#endif
				-> jptype__GenericJPFault;

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

static struct jptype__Attribute sample_attr[] = {
			{ OWNER, NULL },
			{ TIME, "submitted" },
			{ TAG, "test" },
};

static struct jptype__PrimaryQueryElement sample_query[][5] = {
	{
		{ sample_attr+OWNER, EQUAL, "unknown", NULL },
		{ NULL, 0, NULL, NULL }
	},
};

int main(int argc,char *argv[])
{
	char	*server = "http://localhost:8901";
	int	opt;
	struct soap	*soap = soap_new();

	if (argc < 2) usage(argv[0]); 

	soap_init(soap);
	soap_set_namespaces(soap, jpps__namespaces);

	while ((opt = getopt(argc,argv,"s:")) >= 0) switch (opt) {
		case 's': server = optarg;
			  argv += 2;
			break;
		case '?': usage(argv[0]);
	}

	if (!strcasecmp(argv[1],"RegisterJob")) {
		struct jpsrv__RegisterJobResponse	r;

		if (argc != 3) usage(argv[0]);
		check_fault(soap,
			soap_call_jpsrv__RegisterJob(soap,server,"",argv[2],&r));
	} else if (!strcasecmp(argv[1], "StartUpload")) {
		struct jpsrv__StartUploadResponse r;

		if (argc != 6) usage(argv[0]);
		if (!check_fault(soap,
				soap_call_jpsrv__StartUpload(soap, server, "",
					argv[2], atoi(argv[3]), atoi(argv[4]), argv[5], &r))) {
			printf("Destination: %s\nCommit before: %ld\n", r.destination, (long)r.commitBefore);
		}
	} else if (!strcasecmp(argv[1],"FeedIndex")) {
		struct jpsrv__FeedIndexResponse	r;
		struct jptype__Attribute	*ap[2];
		struct jptype__Attributes	attr = { 2, ap };
		struct jptype__PrimaryQueryElement *qp[100];
		struct jptype__PrimaryQuery	qry = { 0, qp }; 

		int	i,j,qi = atoi(argv[3])-1;

		if (argc != 6) usage(argv[0]);

		for (i=0; i<attr.__sizeitem; i++) ap[i] = sample_attr+i;

		for (i=0; sample_query[qi][i].attr; i++)
			qp[i] = &sample_query[qi][i];
		qry.__sizeitem = i;
		
		if (!check_fault(soap,soap_call_jpsrv__FeedIndex(soap,server,"",
				argv[2],&attr,&qry,!strcasecmp(argv[4],"true"),
					!strcasecmp(argv[5],"true"),
					&r)))
		{
			printf("FeedId: %s\nExpires: %s\n",r.feedId,ctime(&r.expires));
		}
	}
	else usage(argv[0]);

	return 0;
}


/* XXX: we don't use it */
SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };
