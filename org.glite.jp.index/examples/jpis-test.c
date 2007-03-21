#include <stdio.h>
#include <sysexits.h>
#include <string.h>
#include <assert.h>

#include <glite/security/glite_gsplugin.h>
#include "glite/jp/strmd5.h"

#include "jpis_H.h"
#include "jpis_.nsmap"
#include "db_ops.h"
#include "conf.h"


#include "soap_version.h"
#if GSOAP_VERSION <= 20602
#define soap_call___jpsrv__UpdateJobs soap_call___ns1__UpdateJobs
#define soap_call___jpsrv__QueryJobs soap_call___ns1__QueryJobs
#endif


/* insert simulating FeedIndex call */
#define INSERT "insert into feeds value ('93', '12345', '8', '0' , 'http://localhost:8901', '2005-10-14 10:48:27', 'COND2');" 
#define DELETE "delete from feeds where feedid = '12345';" 

static int check_fault(struct soap *soap,int err);


	
int main(int argc,char *argv[])
{
	char *default_server = NULL;
	char server[512];
	struct soap	*soap = soap_new();

	soap_init(soap);	
	soap_set_namespaces(soap, jpis__namespaces);
	soap_register_plugin(soap,glite_gsplugin);

/*---------------------------------------------------------------------------*/
	// simulate FeedIndex PS response
	{
		glite_jp_db_stmt_t      stmt;
		glite_jp_context_t      ctx;
		glite_jpis_context_t	isctx;
		glite_jp_is_conf        *conf;
		

		glite_jp_init_context(&ctx);
		glite_jp_get_conf(argc, argv, NULL, &conf);
		if (default_server) strcpy(server, default_server);
		else snprintf(server, sizeof(server), "http://localhost:%s", conf->port ? conf->port : GLITE_JPIS_DEFAULT_PORT_STR);
		printf("JP index server: %s\n", server);

		glite_jpis_init_context(&isctx, ctx, conf);
		if (glite_jpis_init_db(isctx) != 0) {
			fprintf(stderr, "Connect DB failed: %s (%s)\n", 
				ctx->error->desc, ctx->error->source);
			goto end;
		}
		
		if (glite_jp_db_execstmt(ctx, DELETE, &stmt) < 0) goto end;
		if (glite_jp_db_execstmt(ctx, INSERT, &stmt) < 0) goto end;
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

		memset(&in, 0, sizeof(in));
		memset(&out, 0, sizeof(out));

		in.feedId = soap_strdup(soap, "12345");
		in.feedDone = false_;
		in.__sizejobAttributes = 2;
		in.jobAttributes = soap_malloc(soap, 
			in.__sizejobAttributes * sizeof(*(in.jobAttributes)));
		{
			rec = soap_malloc(soap,  sizeof(*rec));
			memset(rec, 0, sizeof(*rec));
			rec->jobid = soap_strdup(soap, "https://localhost:7846/pokus1");
			{
				gss_cred_id_t		cred = GSS_C_NO_CREDENTIAL;
				edg_wll_GssStatus	gss_code;
				char			*subject = NULL;

				if ( edg_wll_gss_acquire_cred_gsi(NULL, NULL, &cred, &subject, &gss_code) ) {
					printf("Cannot obtain credentials - exiting.\n");
					return EINVAL;
				}
				rec->owner = soap_strdup(soap, subject);
				free(subject);
			}
			rec->__sizeprimaryStorage = 0;
			rec->primaryStorage = NULL;
			rec->__sizeattributes = 2;
			rec->attributes = soap_malloc(soap,
				rec->__sizeattributes * sizeof(*(rec->attributes)));
			rec->attributes[0] = soap_malloc(soap, sizeof(*(rec->attributes[0])));
			rec->attributes[0]->name = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:user");
			rec->attributes[0]->value =  soap_malloc(soap, sizeof(*(rec->attributes[0]->value)));
			rec->attributes[0]->value->string = soap_strdup(soap, "CertSubj");
			rec->attributes[0]->value->blob = NULL;
			rec->attributes[0]->timestamp = 333;
			rec->attributes[0]->origin = jptype__attrOrig__SYSTEM;
			rec->attributes[0]->originDetail = NULL;

			rec->attributes[1] = soap_malloc(soap, sizeof(*(rec->attributes[1])));
			rec->attributes[1]->name = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus");
			rec->attributes[1]->value =  soap_malloc(soap, sizeof(*(rec->attributes[0]->value)));
			rec->attributes[1]->value->string = soap_strdup(soap, "Done");
			rec->attributes[1]->value->blob = NULL;
			rec->attributes[1]->timestamp = 333;
			rec->attributes[1]->origin = jptype__attrOrig__SYSTEM;
			rec->attributes[1]->originDetail = NULL;

		}
		in.jobAttributes[0] = rec;
		{
			rec = soap_malloc(soap,  sizeof(*rec));
			memset(rec, 0, sizeof(*rec));
			rec->jobid = soap_strdup(soap, "https://localhost:7846/pokus2");
			rec->owner = soap_strdup(soap, "OwnerName");
			rec->__sizeprimaryStorage = 0;
			rec->primaryStorage = NULL;
			rec->__sizeattributes = 2;
			rec->attributes = soap_malloc(soap,
				rec->__sizeattributes * sizeof(*(rec->attributes)));
			rec->attributes[0] = soap_malloc(soap, sizeof(*(rec->attributes[0])));
			rec->attributes[0]->name = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:user");
			rec->attributes[0]->value =  soap_malloc(soap, sizeof(*(rec->attributes[0]->value)));
			rec->attributes[0]->value->string = soap_strdup(soap, "CertSubj");
			rec->attributes[0]->value->blob = NULL;
			rec->attributes[0]->timestamp = 333;
			rec->attributes[0]->origin = jptype__attrOrig__USER;
			rec->attributes[0]->originDetail = NULL;
			rec->attributes[1] = soap_malloc(soap, sizeof(*(rec->attributes[1])));
			rec->attributes[1]->name = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus");
			rec->attributes[1]->value =  soap_malloc(soap, sizeof(*(rec->attributes[0]->value)));
			rec->attributes[1]->value->string = soap_strdup(soap, "Ready");
			rec->attributes[1]->value->blob = NULL;
			rec->attributes[1]->timestamp = 333;
			rec->attributes[1]->origin = jptype__attrOrig__SYSTEM;
			rec->attributes[1]->originDetail = NULL;
		}
		in.jobAttributes[1] = rec;


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
		int					i, j;

		
		in.__sizeconditions = 2;
		in.conditions = soap_malloc(soap,
			in.__sizeconditions * 
			sizeof(*(in.conditions)));
		
		// query status
		cond = soap_malloc(soap, sizeof(*cond));
		memset(cond, 0, sizeof(*cond));
		cond->attr = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus");
		cond->origin = NULL;
		cond->__sizerecord = 2;
		cond->record = soap_malloc(soap, cond->__sizerecord * sizeof(*(cond->record)));

		// equal to Done
		rec = soap_malloc(soap, sizeof(*rec));
		memset(rec, 0, sizeof(*rec));
		rec->op = jptype__queryOp__EQUAL;
		rec->value = soap_malloc(soap, sizeof(*(rec->value)));
		rec->value->string = soap_strdup(soap, "Done");
		rec->value->blob = NULL;
		cond->record[0] = rec;

		// OR equal to Ready
		rec = soap_malloc(soap, sizeof(*rec));
		memset(rec, 0, sizeof(*rec));
		rec->op = jptype__queryOp__EQUAL;
		rec->value = soap_malloc(soap, sizeof(*(rec->value)));
		rec->value->string = soap_strdup(soap, "Ready");
		rec->value->blob = NULL;
		cond->record[1] = rec;

		in.conditions[0] = cond;

		// AND
		// owner
		cond = soap_malloc(soap, sizeof(*cond));
		memset(cond, 0, sizeof(*cond));
		cond->attr = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:user");
		cond->origin = NULL;
		cond->__sizerecord = 1;
		cond->record = soap_malloc(soap, cond->__sizerecord * sizeof(*(cond->record)));

		// not equal to CertSubj
		rec = soap_malloc(soap, sizeof(*rec));
		memset(rec, 0, sizeof(*rec));
		rec->op = jptype__queryOp__UNEQUAL;
		rec->value = soap_malloc(soap, sizeof(*(rec->value)));
		rec->value->string = soap_strdup(soap, "God");
		rec->value->blob = NULL;
		cond->record[0] = rec;

		in.conditions[1] = cond;


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
			printf("jobid = %s\n",out.jobs[j]->jobid);
			for (i=0; i<out.jobs[j]->__sizeattributes; i++) {
				printf("\t%s = %s\n",
					out.jobs[j]->attributes[i]->name,
					out.jobs[j]->attributes[i]->value->string);
			}
		}
	} 

	return 0;
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



/* XXX: we don't use it */
SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };
