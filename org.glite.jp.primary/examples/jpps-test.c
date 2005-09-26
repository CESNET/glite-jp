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
			"		RecordTag jobid tagname stringvalue\n"
			"		GetJobFiles jobid\n"
			"		GetJobAttr jobid attr\n"
			"		FeedIndex \n"
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

static const char *orig2str(enum jptype__attrOrig orig)
{
	switch (orig) {
		case jptype__attrOrig__SYSTEM: return "SYSTEM";
		case jptype__attrOrig__USER: return "USER";
		case jptype__attrOrig__FILE_: return "FILE";
		default: return "unknown";
	}
}

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
		in.commitBefore = atoi(argv[4]) + time(NULL);
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
		struct jptype__stringOrBlob	val;
		
		int seq = 0;
	
		if (argc != 5) usage(argv[0]);
		
		in.jobid = argv[2];
		in.tag = &tagval;
		tagval.name = argv[3];
		tagval.value = &val;
		val.string = argv[4];
		val.blob = NULL;
		
		if (!check_fault(soap,
				soap_call___jpsrv__RecordTag(soap, server, "",&in, &empty))) {
			/* OK */
		}
	} 
   	 else if (!strcasecmp(argv[1],"FeedIndex")) {
		char  	*ap[2] = {
			"http://egee.cesnet.cz/en/Schema/LB/Attributes:RB",
			"http://egee.cesnet.cz/en/WSDL/jp-system:owner"
		};

		struct jptype__stringOrBlob vals[] = {
			{ "/O=CESNET/O=Masaryk University/CN=Ales Krenek", NULL },
			{ "Done", NULL }
		};

		struct jptype__primaryQuery	q[] = {
			{ 
				"http://egee.cesnet.cz/en/WSDL/jp-system:owner",
				jptype__queryOp__EQUAL,
				NULL, vals, NULL
			},
			{
				"http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus",
				jptype__queryOp__UNEQUAL,
				NULL, vals+1, NULL
			}
		}, *qp[] = { q, q+1 };
		struct _jpelem__FeedIndex	in = {
			"http://some.index//",
			2,ap,
			2,qp,
			0,
			1
		};
		struct _jpelem__FeedIndexResponse	out;

		if (!check_fault(soap,soap_call___jpsrv__FeedIndex(soap,server,"",&in,&out)))
		{
			printf("FeedId: %s\nExpires: %s\n",out.feedId,ctime(&out.feedExpires));
		}
	 }
/* FIXME: new wsdl  */
#if 0
	} else if (!strcasecmp(argv[1], "FeedIndexRefresh")) {
		struct jpsrv__FeedIndexRefreshResponse r;

		if (argc != 3) usage(argv[0]);
		if (!check_fault(soap,
				soap_call_jpsrv__FeedIndexRefresh(soap, server, "",
					argv[2], &r))) {
			printf("FeedId: %s\nExpires: %s\n",r.feedId,ctime(&r.expires));
		}
	}
#endif
	else if (!strcasecmp(argv[1],"GetJobFiles")) {
		struct _jpelem__GetJobFiles	in;
		struct _jpelem__GetJobFilesResponse	out;

		if (argc != 3) usage(argv[0]);
		in.jobid = argv[2];
		
		if (!check_fault(soap,soap_call___jpsrv__GetJobFiles(soap,server,"",
						&in,&out)))
		{
			int	i;

			printf("JobFiles:\n");

			for (i=0; i<out.__sizefiles;i++) {
				printf("\tclass = %s, name = %s, url = %s\n",
						out.files[i]->class_,
						out.files[i]->name,
						out.files[i]->url);
			}
		}

	}
	else if (!strcasecmp(argv[1],"GetJobAttr")) {
		struct _jpelem__GetJobAttributes	in;
		struct _jpelem__GetJobAttributesResponse	out;
		
		if (argc != 4) usage(argv[0]);
		in.jobid = argv[2];
		in.__sizeattributes = 1;
		in.attributes = &argv[3];

		if (!check_fault(soap,soap_call___jpsrv__GetJobAttributes(soap,server,"",&in,&out)))
		{
			int	i;

			puts("Attribute values:");
			for (i=0; i<out.__sizeattrValues; i++)
				printf("\t%s\t%s\t%s",
					out.attrValues[i]->value->string ?
						out.attrValues[i]->value->string :
						"binary",
					orig2str(out.attrValues[i]->origin),
					ctime(&out.attrValues[i]->timestamp));

		}
		
	}
	else usage(argv[0]);

	return 0;
}


/* XXX: we don't use it */
SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };
