/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners/ for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <stdio.h>
#include <sysexits.h>
#include <string.h>
#include <assert.h>

#include <glite/security/glite_gsplugin.h>
#include <glite/security/glite_gss.h>
#include "glite/jobid/strmd5.h"

#include "jp_H.h"
#include "jp_.nsmap"
#include "db_ops.h"
#include "conf.h"


#include "soap_version.h"
#if GSOAP_VERSION <= 20602
#define soap_call___jpsrv__UpdateJobs soap_call___ns1__UpdateJobs
#define soap_call___jpsrv__QueryJobs soap_call___ns1__QueryJobs
#endif
#define dprintf(FMT, ARGS...) fprintf(stderr, FMT, ##ARGS);
#define check_fault(SOAP, ERR) glite_jp_clientCheckFault((SOAP), (ERR), NULL, 0)
#include "glite/jp/ws_fault.c"


/* insert simulating FeedIndex call */
#define INSERT "insert into feeds value ('93', '12345', '8', '0' , 'http://localhost:8901', '2005-10-14 10:48:27', 'COND2');" 
#define DELETE "delete from feeds where feedid = '12345';" 

	
int main(int argc,char *argv[])
{
	char *default_server = NULL;
	char server[512];
	struct soap	*soap = soap_new();

	soap_init(soap);	
	soap_set_namespaces(soap, jp__namespaces);
	soap_register_plugin(soap,glite_gsplugin);

/*---------------------------------------------------------------------------*/
	// simulate FeedIndex PS response
	{
		glite_jp_context_t      ctx;
		glite_jpis_context_t	isctx = NULL;
		glite_jp_is_conf        *conf;
		

		glite_jp_init_context(&ctx);
		glite_jp_get_conf(argc, argv, &conf);
		if (!conf) {
			fprintf(stderr, "Can't gather configuration\n");
			goto end;
		}
		if (default_server) strcpy(server, default_server);
		else snprintf(server, sizeof(server), "http://localhost:%s", conf->port ? conf->port : GLITE_JPIS_DEFAULT_PORT_STR);
		printf("JP index server: %s\n", server);

		glite_jpis_init_context(&isctx, ctx, conf);
		if (glite_jpis_init_db(isctx) != 0) {
			fprintf(stderr, "Connect DB failed: %s (%s)\n", 
				ctx->error->desc, ctx->error->source);
			goto end;
		}
		
		if (glite_jp_db_ExecSQL(ctx, DELETE, NULL) < 0) goto end;
		if (glite_jp_db_ExecSQL(ctx, INSERT, NULL) < 0) goto end;
	end:
		glite_jpis_free_context(isctx);
		glite_jp_free_context(ctx);
		glite_jp_free_conf(conf);
	}

/*---------------------------------------------------------------------------*/
	// test calls of server functions
	// this call is issued by JPPS
	{
		struct jptype__jobRecord		*rec;
		struct _jpelem__UpdateJobs 		in;
		struct _jpelem__UpdateJobsResponse 	out;
		struct jptype__attrValue *a;

		memset(&in, 0, sizeof(in));
		memset(&out, 0, sizeof(out));

		in.feedId = soap_strdup(soap, "12345");
		in.feedDone = GLITE_SECURITY_GSOAP_FALSE;
		GLITE_SECURITY_GSOAP_LIST_CREATE(soap, &in, jobAttributes, struct jptype__jobRecord, 2);
		rec = GLITE_SECURITY_GSOAP_LIST_GET(in.jobAttributes, 0);
		{
			memset(rec, 0, sizeof(*rec));
			rec->jobid = soap_strdup(soap, "https://localhost:7846/pokus1");
			{
				edg_wll_GssCred		cred = NULL;
				edg_wll_GssStatus	gss_code;

				if ( edg_wll_gss_acquire_cred_gsi(NULL, NULL, &cred, &gss_code) ) {
					printf("Cannot obtain credentials - exiting.\n");
					return EINVAL;
				}
				rec->owner = soap_strdup(soap, cred->name);
			}
			rec->__sizeprimaryStorage = 0;
			rec->primaryStorage = NULL;
			GLITE_SECURITY_GSOAP_LIST_CREATE(soap, rec, attributes, struct jptype__attrValue, 2);
			a = GLITE_SECURITY_GSOAP_LIST_GET(rec->attributes, 0);
			a->name = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:user");
			a->value =  soap_malloc(soap, sizeof(*(a->value)));
			memset(a->value, 0, sizeof(a->value));
			GSOAP_SETSTRING(a->value, soap_strdup(soap, "CertSubj"));
			a->timestamp = 333;
			a->origin = jptype__attrOrig__SYSTEM;
			a->originDetail = NULL;

			a = GLITE_SECURITY_GSOAP_LIST_GET(rec->attributes, 1);
			a->name = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus");
			a->value =  soap_malloc(soap, sizeof(*(a->value)));
			memset(a->value, 0, sizeof(a->value));
			GSOAP_SETSTRING(a->value, soap_strdup(soap, "Done"));
			a->timestamp = 333;
			a->origin = jptype__attrOrig__SYSTEM;
			a->originDetail = NULL;

		}

		rec = GLITE_SECURITY_GSOAP_LIST_GET(in.jobAttributes, 1);
		{
			memset(rec, 0, sizeof(*rec));
			rec->jobid = soap_strdup(soap, "https://localhost:7846/pokus2");
			rec->owner = soap_strdup(soap, "OwnerName");
			rec->__sizeprimaryStorage = 0;
			rec->primaryStorage = NULL;
			GLITE_SECURITY_GSOAP_LIST_CREATE(soap, rec, attributes, struct jptype__jobRecord, 2);
			a = GLITE_SECURITY_GSOAP_LIST_GET(rec->attributes, 0);
			a->name = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:user");
			a->value =  soap_malloc(soap, sizeof(*(a->value)));
			memset(a->value, 0, sizeof(a->value));
			GSOAP_SETSTRING(a->value, soap_strdup(soap, "CertSubj"));
			a->timestamp = 333;
			a->origin = jptype__attrOrig__USER;
			a->originDetail = NULL;

			a = GLITE_SECURITY_GSOAP_LIST_GET(rec->attributes, 1);
			a->name = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus");
			a->value =  soap_malloc(soap, sizeof(*(a->value)));
			memset(a->value, 0, sizeof(a->value));
			GSOAP_SETSTRING(a->value, soap_strdup(soap, "Ready"));
			a->timestamp = 333;
			a->origin = jptype__attrOrig__SYSTEM;
			a->originDetail = NULL;
		}

		check_fault(soap,
			soap_call___jpsrv__UpdateJobs(soap,server,"",&in,&out));
	}

/*---------------------------------------------------------------------------*/
	// this query call issued by user
	{
		struct _jpelem__QueryJobs		in;
		struct jptype__indexQuery 		*cond;
		struct jptype__indexQueryRecord 	*rec;
		struct _jpelem__QueryJobsResponse	out;
		struct jptype__jobRecord		*job;
		struct jptype__attrValue		*attr;
		int					i, j;

		GLITE_SECURITY_GSOAP_LIST_CREATE(soap, &in, conditions, struct jptype__indexQuery, 2);
		
		// query status
		cond = GLITE_SECURITY_GSOAP_LIST_GET(in.conditions, 0);
		memset(cond, 0, sizeof(*cond));
		cond->attr = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus");
		cond->origin = NULL;
		GLITE_SECURITY_GSOAP_LIST_CREATE(soap, cond, record, struct jptype__indexQueryRecord, 2);

		// equal to Done
		rec = GLITE_SECURITY_GSOAP_LIST_GET(cond->record, 0);
		memset(rec, 0, sizeof(*rec));
		rec->op = jptype__queryOp__EQUAL;
		rec->value = soap_malloc(soap, sizeof(*(rec->value)));
		memset(rec->value, 0, sizeof(*rec->value));
		GSOAP_SETSTRING(rec->value, soap_strdup(soap, "Done"));

		// OR equal to Ready
		rec = GLITE_SECURITY_GSOAP_LIST_GET(cond->record, 1);
		memset(rec, 0, sizeof(*rec));
		rec->op = jptype__queryOp__EQUAL;
		rec->value = soap_malloc(soap, sizeof(*(rec->value)));
		memset(rec->value, 0, sizeof(*rec->value));
		GSOAP_SETSTRING(rec->value, soap_strdup(soap, "Ready"));


		// AND
		// owner
		cond = GLITE_SECURITY_GSOAP_LIST_GET(in.conditions, 1);
		memset(cond, 0, sizeof(*cond));
		cond->attr = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:user");
		cond->origin = NULL;
		GLITE_SECURITY_GSOAP_LIST_CREATE(soap, cond, record, struct jptype__indexQueryRecord, 1);

		// not equal to CertSubj
		rec = GLITE_SECURITY_GSOAP_LIST_GET(cond->record, 0);
		memset(rec, 0, sizeof(*rec));
		rec->op = jptype__queryOp__UNEQUAL;
		rec->value = soap_malloc(soap, sizeof(*(rec->value)));
		memset(rec->value, 0, sizeof(*rec->value));
		GSOAP_SETSTRING(rec->value,  soap_strdup(soap, "God"));


		in.__sizeattributes = 4;
		in.attributes = soap_malloc(soap,
			in.__sizeattributes *
			sizeof(*(in.attributes)));
		in.attributes[0] = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/JP/System:owner");
		in.attributes[1] = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/JP/System:jobId");
		in.attributes[2] = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus");
		in.attributes[3] = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:user");

		memset(&out, 0, sizeof(out));

		check_fault(soap,
			soap_call___jpsrv__QueryJobs(soap, server, "",&in,&out));

		for (j=0; j<out.__sizejobs; j++) {
			job = GLITE_SECURITY_GSOAP_LIST_GET(out.jobs, j);
			printf("jobid = %s\n",job->jobid);
			for (i=0; i<job->__sizeattributes; i++) {
				attr = GLITE_SECURITY_GSOAP_LIST_GET(job->attributes, i);
				printf("\t%s = %s\n",
					attr->name,
					GSOAP_ISSTRING(attr->value) ? GSOAP_STRING(attr->value) : "binary");
			}
		}
	} 

	return 0;
}
