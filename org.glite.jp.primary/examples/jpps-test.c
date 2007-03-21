#include <stdio.h>
#include <sysexits.h>
#include <string.h>
#include <assert.h>

#include "jpps_H.h"
#include "jpps_.nsmap"

#include "jptype_map.h"
#include "glite/security/glite_gsplugin.h"
#include "glite/security/glite_gscompat.h"

#if GSOAP_VERSION <= 20602
#define soap_call___jpsrv__RegisterJob soap_call___ns1__RegisterJob
#define soap_call___jpsrv__StartUpload soap_call___ns1__StartUpload
#define soap_call___jpsrv__CommitUpload soap_call___ns1__CommitUpload
#define soap_call___jpsrv__RecordTag soap_call___ns1__RecordTag
#define soap_call___jpsrv__FeedIndex soap_call___ns1__FeedIndex
#define soap_call___jpsrv__FeedIndexRefresh soap_call___ns1__FeedIndexRefresh
#define soap_call___jpsrv__GetJob soap_call___ns1__GetJob
#endif

#define dprintf(FMT, ARGS...) printf(FMT, ##ARGS)
#include "glite/jp/ws_fault.c"
#define check_fault(SOAP, ERR) glite_jp_clientCheckFault((SOAP), (ERR), NULL, 0)

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
			"		FeedIndex [yes (history)]\n"
			"		FeedIndexRefresh feedid\n"
		,me);

	exit (EX_USAGE);
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
	char	*server = NULL;
	int	opt,ret = 0;
	struct soap	*soap = soap_new();

	if (argc < 2) usage(argv[0]); 

	soap_init(soap);
	soap_set_namespaces(soap, jpps__namespaces);

	soap_register_plugin(soap,glite_gsplugin);

	while ((opt = getopt(argc,argv,"s:")) >= 0) switch (opt) {
		case 's': server = optarg;
			break;
		case '?': usage(argv[0]);
	}

	if (server) {
		argv += 2;
		argc -= 2;
	}
	else server = "http://localhost:8901";

	if (!strcasecmp(argv[1],"RegisterJob")) {
		struct _jpelem__RegisterJob	in;
		struct _jpelem__RegisterJobResponse	empty;

		if (argc != 4) usage(argv[0]);
		in.job = argv[2];
		in.owner = argv[3];
		ret = check_fault(soap,
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
		if (!(ret = check_fault(soap,
				soap_call___jpsrv__StartUpload(soap, server, "",&in,&out))))
		{
			printf("Destination: %s\nCommit before: %s\n", out.destination, ctime(&out.commitBefore));
		}
	} else if (!strcasecmp(argv[1], "CommitUpload")) {
		struct _jpelem__CommitUpload	in;
		struct _jpelem__CommitUploadResponse	empty;

		in.destination = argv[2];

		if (argc != 3) usage(argv[0]);
		if (!(ret = check_fault(soap,
				soap_call___jpsrv__CommitUpload(soap, server, "",&in,&empty)))) {
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
		memset(&val, 0, sizeof(val));
		GSOAP_SETSTRING(&val, argv[4]);
		
		if (!(ret = check_fault(soap,
				soap_call___jpsrv__RecordTag(soap, server, "",&in, &empty)))) {
			/* OK */
		}
	} 
   	 else if (!strcasecmp(argv[1],"FeedIndex")) {
		char  	*ap[2] = {
			"http://egee.cesnet.cz/en/Schema/LB/Attributes:RB",
			"http://egee.cesnet.cz/en/Schema/JP/System:owner"
		};
		int sizepq;

		struct jptype__stringOrBlob vals[2];
		memset(vals, 0, sizeof vals);
		GSOAP_SETSTRING(vals, "/O=CESNET/O=Masaryk University/CN=Ales Krenek");
		GSOAP_SETSTRING(vals + 1, "Done");

		struct jptype__primaryQuery	q[] = {
			{ 
				"http://egee.cesnet.cz/en/Schema/JP/System:owner",
				jptype__queryOp__EQUAL,
				NULL, vals, NULL
			},
			{
				"http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus",
				jptype__queryOp__UNEQUAL,
				NULL, vals+1, NULL
			}
		};
		GLITE_SECURITY_GSOAP_LIST_TYPE(jptype, primaryQuery) pq;

		GLITE_SECURITY_GSOAP_LIST_CREATE0(soap, pq, sizepq, struct jptype__primaryQuery, 2);
		memcpy(GLITE_SECURITY_GSOAP_LIST_GET(pq, 0), &q[0], sizeof(q[0]));
		memcpy(GLITE_SECURITY_GSOAP_LIST_GET(pq, 1), &q[1], sizeof(q[1]));
		struct _jpelem__FeedIndex	in = {
			"http://some.index//",
			2,ap,
			sizepq,pq,
			0,
			1
		};
		struct _jpelem__FeedIndexResponse	out;

		in.history = argc >= 3 && !strcasecmp(argv[2],"yes");

		if (!(ret = check_fault(soap,soap_call___jpsrv__FeedIndex(soap,server,"",&in,&out))))
		{
			printf("FeedId: %s\nExpires: %s\n",out.feedId,ctime(&out.feedExpires));
		}
		GLITE_SECURITY_GSOAP_LIST_DESTROY(soap, &in, conditions);
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
		struct jptype__jppsFile	*outf;

		if (argc != 3) usage(argv[0]);
		in.jobid = argv[2];
		
		if (!(ret = check_fault(soap,soap_call___jpsrv__GetJobFiles(soap,server,"",
						&in,&out))))
		{
			int	i;

			printf("JobFiles:\n");

			for (i=0; i<out.__sizefiles;i++) {
				outf = GLITE_SECURITY_GSOAP_LIST_GET(out.files, i);
				printf("\tclass = %s, name = %s, url = %s\n",
						outf->class_,
						outf->name,
						outf->url);
			}
		}

	}
	else if (!strcasecmp(argv[1],"GetJobAttr")) {
		struct _jpelem__GetJobAttributes	in;
		struct _jpelem__GetJobAttributesResponse	out;
		struct jptype__attrValue	*outav;
		
		if (argc != 4) usage(argv[0]);
		in.jobid = argv[2];
		in.__sizeattributes = 1;
		in.attributes = &argv[3];

		if (!(ret = check_fault(soap,soap_call___jpsrv__GetJobAttributes(soap,server,"",&in,&out))))
		{
			int	i;

			puts("Attribute values:");
			for (i=0; i<out.__sizeattrValues; i++) {
				outav = GLITE_SECURITY_GSOAP_LIST_GET(out.attrValues, i);
				printf("\t%s\t%s\t%s",
					GSOAP_ISSTRING(outav->value) ?
						GSOAP_STRING(outav->value) :
						"binary",
					orig2str(outav->origin),
					ctime(&outav->timestamp));
			}

		}
		
	}
	else { usage(argv[0]); ret = 1; }

	return ret;
}


/* XXX: we don't use it */
SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };
