#define _GNU_SOURCE	/* strndup */

#include <stdio.h>
#include <sysexits.h>

#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <cclassad.h>

#include "glite/jp/known_attr.h"

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

#define dprintf(FMT, ARGS...) fprintf(stderr, (FMT), ##ARGS)
#include "glite/jp/ws_fault.c"
#define check_fault(SOAP, ERR) glite_jp_clientCheckFault((SOAP), (ERR), NULL, 0)


static void usage(const char *me)
{
	fprintf(stderr,"%s: [-s server-url] jobid\n",me);

	exit (EX_USAGE);
}

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
	int	opt,ret = 0,i;
	struct soap	*soap = soap_new();
	struct _jpelem__GetJobAttributes	in;
	struct _jpelem__GetJobAttributesResponse	out;
	char	*aname = "http://egee.cesnet.cz/en/Schema/LB/Attributes:JDL";
	struct cclassad	*ad;
	struct { char *a,*s; } *deps = calloc(1,sizeof *deps);
	int	ndeps = 0;
	char	*dep_s,*where,*end,*tmp_a,*tmp_s,*wa,*wa_r,*ws,*ws_r;

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


		
	in.jobid = argv[1];
	in.__sizeattributes = 1;
	in.attributes = &aname;

	puts("Retrieving JDL ...");
	if ((ret = check_fault(soap,soap_call___jpsrv__GetJobAttributes(soap,server,"",&in,&out))))
		return 1;

       	ad = cclassad_create(GSOAP_STRING(GLITE_SECURITY_GSOAP_LIST_GET(out.attrValues, 0)->value));
	if (!ad) {
		fputs("Can't parse JDL\n",stderr);
		return 1;
	}

	// cclassad_evaluate_to_string(ad,"dependencies",&dep_s);
	cclassad_evaluate_to_expr(ad,"dependencies",&dep_s);

	/* XXX: assumes syntacticly correct dependencies = { ... } */
	where = strchr(dep_s,'{'); assert(where);
	where++;

	while ((where = strchr(where, '{'))) {	/* 2nd level */
		for (where++; isspace(*where); where++);

		if (*where == '{') end = strchr(where, '}')+1;	/* more ancestors */
		else for (end = where; !isspace(*end) && *end != ','; end++);
		tmp_a = strndup(where,end - where);
		where = end++;

		while(isspace(*where)) where++;
		where++;	/* comma */
		while(isspace(*where)) where++;

		if (*where == '{') end = strchr(where, '}')+1;    /* more successors */
		else for (end = where; !isspace(*end) && *end != ','; end++);
		tmp_s = strndup(where,end - where); 
		where = strchr(end+1,'}');
		
#define DELIM "{} ,\t\n"
		for (ws = strtok_r(tmp_s,DELIM,&ws_r); ws; ws = strtok_r(NULL,DELIM,&ws_r)) 
			for (wa = strtok_r(tmp_a,DELIM,&wa_r); wa; wa = strtok_r(NULL,DELIM,&wa_r)) {
				deps[ndeps].a = strdup(wa);
				deps[ndeps].s = strdup(ws);
				deps = realloc(deps, (ndeps+2) * sizeof *deps);
				ndeps++;
				deps[ndeps].a = deps[ndeps].s = NULL;
			}
		free(tmp_a); free(tmp_s); 
	}

	for (i=0; deps[i].a; i++) {
		char	attr[1000],*ja,*js;
		int	have_a,have_s;

		printf("node: %s -> %s\n",deps[i].a,deps[i].s);
		sprintf(attr,"nodes.%s.description.edg_jobid",deps[i].a);
		have_a = cclassad_evaluate_to_string(ad,attr,&ja);

		sprintf(attr,"nodes.%s.description.edg_jobid",deps[i].s);
		have_s = cclassad_evaluate_to_string(ad,attr,&js);

		printf("jobid: %s -> %s\n",ja,js);

		if (have_a && have_s) {
			struct _jpelem__RecordTag	in;
			struct _jpelem__RecordTagResponse	empty;
			struct jptype__tagValue tagval;
			struct jptype__stringOrBlob val;

			in.jobid = ja;
			in.tag = &tagval;
			tagval.name = GLITE_JP_ATTR_WF_SUCCESSOR;
			tagval.value = &val;
			memset(&val, 0, sizeof(val));
			GSOAP_SETSTRING(&val, js);

			printf("Register successor ...\n");
			ret = check_fault(soap,soap_call___jpsrv__RecordTag(soap, server, "",&in, &empty));
			in.jobid = js;
			tagval.name = GLITE_JP_ATTR_WF_ANCESTOR;
			GSOAP_SETSTRING(&val, ja);

			printf("Register ancestor ...\n");
			ret = check_fault(soap,soap_call___jpsrv__RecordTag(soap, server, "",&in, &empty));
			putchar(10);
		}
	}

	return ret;
}
