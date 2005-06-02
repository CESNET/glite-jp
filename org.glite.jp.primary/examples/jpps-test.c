#include <stdio.h>
#include <sysexits.h>
#include <string.h>
#include <assert.h>

#include "glite/security/glite_gsplugin.h"

#include "jpps_H.h"
#include "jpps_.nsmap"

#include "jptype_map.h"

#include "soap_version.h"
#if GSOAP_VERSION <= 20602
#define soap_call___jpsrv__RegisterJob soap_call___ns1__RegisterJob
#define soap_call___jpsrv__StartUpload soap_call___ns1__StartUpload
#define soap_call___jpsrv__CommitUpload soap_call___ns1__CommitUpload
#define soap_call___jpsrv__RecordTag soap_call___ns1__RecordTag
#define soap_call___jpsrv__FeedIndex soap_call___ns1__FeedIndex
#define soap_call___jpsrv__FeedIndexRefresh soap_call___ns1__FeedIndexRefresh
#define soap_call___jpsrv__GetJob soap_call___ns1__GetJob
#endif


static void usage(const char *me)
{
	fprintf(stderr,"%s: [-s server-url] operation args \n\n"
			"	operations are:\n"
			"		RegisterJob jobid owner\n"
			"		StartUpload jobid class commit_before mimetype\n"
			"		CommitUpload destination\n"
			"		RecordTag jobid tagname sequence stringvalue\n"
			"		GetJob jobid\n"
			"		FeedIndex destination query_number history continuous\n"
			"		FeedIndexRefresh feedid\n"
		,me);

	exit (EX_USAGE);
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

/* FIXME: new wsdl */
#if 0
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
#endif

int main(int argc,char *argv[])
{
	char	*server = "http://localhost:8901";
	int	opt;
	struct soap	*soap = soap_new();

	if (argc < 2) usage(argv[0]); 

	soap_init(soap);
	soap_set_namespaces(soap, jpps__namespaces);

	soap_register_plugin(soap,glite_gsplugin);

	while ((opt = getopt(argc,argv,"s:")) >= 0) switch (opt) {
		case 's': server = optarg;
			  argv += 2;
			break;
		case '?': usage(argv[0]);
	}

	if (!strcasecmp(argv[1],"RegisterJob")) {
		struct _jpelem__RegisterJob	in;
		struct _jpelem__RegisterJobResponse	empty;

		if (argc != 4) usage(argv[0]);
		in.job = argv[2];
		in.owner = argv[3];
		check_fault(soap,
			soap_call___jpsrv__RegisterJob(soap,server,"",&in,&empty));
	} else if (!strcasecmp(argv[1], "StartUpload")) {
		struct _jpelem__StartUpload		in;
		struct _jpelem__StartUploadResponse	out;

		in.job = argv[2];
		in.class_ = argv[3];
		in.name = NULL;
		in.commitBefore = atoi(argv[4]);
		in.contentType = argv[5];

		if (argc != 6) usage(argv[0]);
		if (!check_fault(soap,
				soap_call___jpsrv__StartUpload(soap, server, "",&in,&out)))
		{
			printf("Destination: %s\nCommit before: %s\n", out.destination, ctime(&out.commitBefore));
		}
	} else if (!strcasecmp(argv[1], "CommitUpload")) {
		struct _jpelem__CommitUpload	in;
		struct _jpelem__CommitUploadResponse	empty;

		in.destination = argv[2];

		if (argc != 3) usage(argv[0]);
		if (!check_fault(soap,
				soap_call___jpsrv__CommitUpload(soap, server, "",&in,&empty))) {
			/* OK */
		}
	} else if (!strcasecmp(argv[1], "RecordTag")) {
		struct _jpelem__RecordTag	in;
		struct _jpelem__RecordTagResponse	empty;
		struct jptype__tagValue tagval;
		
		if (argc != 6) usage(argv[0]);
		
		in.jobid = argv[2];
		in.tag = &tagval;
		tagval.name = argv[3];
		tagval.sequence = NULL;
		tagval.timestamp = NULL;
		tagval.stringValue = argv[5];
		tagval.blobValue = NULL;
		
		if (!check_fault(soap,
				soap_call___jpsrv__RecordTag(soap, server, "",&in, &empty))) {
			/* OK */
		}
	} 
/* FIXME: new wsdl  */
#if 0
   	 else if (!strcasecmp(argv[1],"FeedIndex")) {
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
	} else if (!strcasecmp(argv[1], "FeedIndexRefresh")) {
		struct jpsrv__FeedIndexRefreshResponse r;

		if (argc != 3) usage(argv[0]);
		if (!check_fault(soap,
				soap_call_jpsrv__FeedIndexRefresh(soap, server, "",
					argv[2], &r))) {
			printf("FeedId: %s\nExpires: %s\n",r.feedId,ctime(&r.expires));
		}
	} else if (!strcasecmp(argv[1],"GetJob")) {
		struct jpsrv__GetJobResponse	r;

		if (argc != 3) usage(argv[0]);
		
		if (!check_fault(soap,soap_call_jpsrv__GetJob(soap,server,"",
						argv[2],&r)))
		{
			int	i;

			printf("JobLog:\n");

			for (i=0; i<r.files->__sizefile;i++) {
				printf("\tclass = %s, name = %s, url = %s\n",
						r.files->file[i]->class_,
						r.files->file[i]->name,
						r.files->file[i]->url);
			}
		}

	}
#endif
	else usage(argv[0]);

	return 0;
}


/* XXX: we don't use it */
SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };
