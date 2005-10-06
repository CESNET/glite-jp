#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/strmd5.h"
#include "glite/jp/attr.h"
#include "glite/lb/trio.h"

#include "jpis_H.h"
#include "jpis_.nsmap"
#include "soap_version.h"
#include "db_ops.h"
// XXX: avoid 2 wsdl collisions - work only because ws_ps_typeref.h 
// uses common types from jpis_H.h (awful)
#include "ws_ps_typeref.h"
#include "ws_is_typeref.h"
#include "context.h"

#define	INDEXED_STRIDE	2	// how often realloc indexed attr result
				// XXX: 2 is only for debugging, replace with e.g. 100
#define	JOBIDS_STRIDE	2	// how often realloc matched jobids result

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





/*-----------------------------------------*/
/* IS WSDL server function implementations */
/*-----------------------------------------*/


#define CONTEXT_FROM_SOAP(soap,ctx) glite_jpis_context_t	ctx = (glite_jpis_context_t) ((soap)->user)


static int updateJob(glite_jpis_context_t isctx, struct jptype__jobRecord *jobAttrs) {
	glite_jp_attrval_t av;
	struct jptype__attrValue *attr;
	int ret, iattrs;

	printf("%s: jobid='%s', attrs=%d\n", __FUNCTION__, jobAttrs->jobid, jobAttrs->__sizeattributes);

	if (jobAttrs->remove) assert(*(jobAttrs->remove) == 0);

	for (iattrs = 0; iattrs < jobAttrs->__sizeattributes; iattrs++) {
		attr = jobAttrs->attributes[iattrs];
		glite_jpis_SoapToAttrVal(&av, attr);
		if ((ret = glite_jpis_insertAttrVal(isctx, jobAttrs->jobid, &av)) != 0) return ret;
	}

	return 0;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__UpdateJobs(
	struct soap *soap,
	struct _jpelem__UpdateJobs *jpelem__UpdateJobs,
	struct _jpelem__UpdateJobsResponse *jpelem__UpdateJobsResponse)
{
	int ret, ijobs;
	const char *feedid;
	int status, done;
	CONTEXT_FROM_SOAP(soap, isctx);
	glite_jp_context_t jpctx = isctx->jpctx;

	// XXX: test client in examples/jpis-test
	//      sends to this function some data for testing
	puts(__FUNCTION__);

	// get info about the feed
	feedid = jpelem__UpdateJobs->feedId;
	memset(isctx->param_feedid, 0, sizeof(isctx->param_feedid));
	strncpy(isctx->param_feedid, feedid, sizeof(isctx->param_feedid) - 1);
	if ((ret = glite_jp_db_execute(isctx->select_info_feed_stmt)) != 1) {
		fprintf(stderr, "can't get info about '%s', returned %d records: %s (%s)\n", feedid, ret, jpctx->error->desc, jpctx->error->source);
		return SOAP_FAULT;
	}
	// update status, if needed (only oring)
	status = isctx->param_state;
	done = jpelem__UpdateJobs->feedDone ? GLITE_JP_IS_STATE_DONE : 0;
	if ((done != (status & GLITE_JP_IS_STATE_DONE)) && done) {
		isctx->param_state |= done;
		if ((ret = glite_jp_db_execute(isctx->update_state_feed_stmt)) != 1) {
			fprintf(stderr, "can't update state of '%s', returned %d records: %s (%s)\n", feedid, ret, jpctx->error->desc, jpctx->error->source);
			return SOAP_FAULT;
		}
	}

	// insert all attributes
	for (ijobs = 0; ijobs < jpelem__UpdateJobs->__sizejobAttributes; ijobs++) {
		if (updateJob(isctx, jpelem__UpdateJobs->jobAttributes[ijobs]) != 0) return SOAP_FAULT;
	}

	return SOAP_OK;
}


static int checkIndexedConditions(glite_jpis_context_t isctx, struct _jpelem__QueryJobs *in)
{
	char 			**indexed_attrs;
	int			i, j, k, ret;


	if ((ret = glite_jp_db_execute(isctx->select_info_attrs_indexed)) == -1) {
		fprintf(stderr, "Error when executing select_info_attrs_indexed, \
			returned %d records: %s (%s)\n", 
			ret, isctx->jpctx->error->desc, isctx->jpctx->error->source);
		return SOAP_FAULT;
	}

	i = 0;
	while (glite_jp_db_fetch(isctx->select_info_attrs_indexed) == 0) {
		if (!(i % INDEXED_STRIDE)) {
			indexed_attrs = realloc(indexed_attrs, 
				((i / INDEXED_STRIDE) * INDEXED_STRIDE + 1)  
				* sizeof(*indexed_attrs));
		}
		i++;
		indexed_attrs[i] = strdup(isctx->param_indexed);
        }

	for (k=0; k < in->__sizeconditions; k++) {
		for (j=0; j < i; j++) {
			if (!strcmp(in->conditions[k]->attr, indexed_attrs[j])) {
				ret = 0;
				goto end;
			}
		}
	} 
	ret = 1;

end:
	for (j=0;  j < i; j++) free(indexed_attrs[j]);
	free(indexed_attrs);

	return(ret);
}


static int get_jobids(struct soap *soap, glite_jpis_context_t isctx, struct _jpelem__QueryJobs *in, char ***jobids, char *** ps_list)
{
	char 			*qa, *qb, *qop, *qbase, *query, *res[2], **jids, **pss;
	int 			i, j, ret;
	glite_jp_db_stmt_t	stmt;
	glite_jp_queryop_t	op;
	glite_jp_attrval_t	attr;


	trio_asprintf(&qbase,"SELECT dg_jobid FROM jobs WHERE ");
	
	for (i=0; i < in->__sizeconditions; i++) {
		if (i == 0) {
			trio_asprintf(&qa,"jobs.jobid = attr_%|Ss.jobid AND (", 
				str2md5(in->conditions[i]->attr));
		}
		else {
			trio_asprintf(&qb,"%s AND jobs.jobid = attr_%|Ss.jobid AND (", 
				qa, str2md5(in->conditions[i]->attr));
			free(qa); qa = qb; qb = NULL;
		}	
	
		for (j=0; j < in->conditions[i]->__sizerecord; j++) { 
			glite_jpis_SoapToQueryOp(in->conditions[i]->record[j]->op, &op);
			switch (op) {
				case GLITE_JP_QUERYOP_EQUAL:
					qop = strdup("=");
					break;
				case  GLITE_JP_QUERYOP_UNEQUAL:
					qop = strdup("!=");
					break;
				default:
					// unsupported query operator
					goto err;
					break;
			}
			if (in->conditions[i]->record[j]->value->string) {
				attr.name = in->conditions[i]->attr;
				attr.value = in->conditions[i]->record[j]->value->string;
				attr.binary = 0;
				glite_jpis_SoapToAttrOrig(soap,
					in->conditions[i]->origin, &(attr.origin));
				trio_asprintf(&qb,"%s OR attr_%|Ss.value %s %|Ss ",
					qa, in->conditions[i]->attr, qop,
					glite_jp_attrval_to_db_index(isctx->jpctx, &attr, 255));
				free(qop);
				free(qa); qa = qb; qb = NULL;
			}
			else {
				attr.name = in->conditions[i]->attr;
				attr.value = in->conditions[i]->record[j]->value->blob->__ptr;
				attr.binary = 1;
				attr.size = in->conditions[i]->record[j]->value->blob->__size;
				glite_jpis_SoapToAttrOrig(soap,
					in->conditions[i]->origin, &(attr.origin));
				trio_asprintf(&qb,"%s OR attr_%|Ss.value %s %|Ss ",
					qa, in->conditions[i]->attr, qop,
					glite_jp_attrval_to_db_index(isctx->jpctx, &attr, 255));
				free(qop);
				free(qa); qa = qb; qb = NULL; 
			}
		}
		trio_asprintf(&qb,"%s)",qa);
		free(qa); qa = qb; qb = NULL;
	}
	trio_asprintf(&query, "%s%s;", qbase, qa);
	free(qbase);
	free(qa);
	
	// XXX: add where's for attr origin (not clear now whether stored in separate column
	// or obtained via glite_jp_attrval_from_db...

	if ((ret = glite_jp_db_execstmt(isctx->jpctx, query, &stmt)) < 0) goto err;
	free(query);

	i = 0;
	do {
		if ( (ret = glite_jp_db_fetchrow(stmt, res) < 0) ) goto err;
		if (!(i % JOBIDS_STRIDE)) {
                        jids = realloc(jids,
                                ((i / JOBIDS_STRIDE) * JOBIDS_STRIDE + 2)
                                * sizeof(*jids));
                }
		if (!(i % JOBIDS_STRIDE)) {
                        pss = realloc(pss,
                                ((i / JOBIDS_STRIDE) * JOBIDS_STRIDE + 2)
                                * sizeof(*pss));
                }
		jids[i] = res[0];
		jids[i+1] = NULL;
		pss[i] = res[1];
		pss[i+1] = NULL;
		i++;	
	} while (ret);
	glite_jp_db_freestmt(&stmt);	

	*jobids = jids;
	*ps_list = pss;

	return 0;
	
err:
	free(query);
	free(pss);
	free(jids);
	glite_jp_db_freestmt(&stmt);

	return ret;
}

static void freeAttval_t(glite_jp_attrval_t jav)
{
	free(jav.name);
	free(jav.value);
	free(jav.origin_detail);
}


/* get all values of a given attribute for a job with a given jobid 	*/
/* all values are soap_malloc-ated, exept of av (due to absence of 	*/
/* soap_realloc)							*/ 
/* Needs to be reallocated with soap_malloc in calling function!	*/

static int get_attr(struct soap *soap, glite_jpis_context_t isctx, char *jobid, char *attr_name, struct jptype__jobRecord *out)
{
	glite_jp_attrval_t		jav;
	struct jptype__attrValue	**av;
	enum jptype__attrOrig		*origin;
	char 				*query, *fv;
	int 				i, ret;
	glite_jp_db_stmt_t      	stmt;


	trio_asprintf(&query,"SELECT full_value FROM attr_%|Ss WHERE jobid = %s",
		attr_name, jobid);

	if ((ret = glite_jp_db_execstmt(isctx->jpctx, query, &stmt)) < 0) goto err;
	free(query);

	i = 0;
	do {
		if ( (ret = glite_jp_db_fetchrow(stmt, &fv) < 0) ) goto err;
		i++;	
		av = realloc(av, i * sizeof(*av));
		av[i] = soap_malloc(soap, sizeof(*av[i]));

		if (glite_jp_attrval_from_db(isctx->jpctx, fv, &jav)) goto err;
		av[i]->name = soap_strdup(soap, jav.name);
		if (jav.binary) {
			av[i]->value->blob = soap_malloc(soap, sizeof(*(av[i]->value->blob)));
			av[i]->value->blob->__ptr = soap_malloc(soap, jav.size);
			memcpy(av[i]->value->blob->__ptr, jav.value, jav.size);
			av[i]->value->blob->__size = jav.size;
			// XXX: id, type, option - how to handle?
		}
		else {
			av[i]->value->string = soap_strdup(soap, jav.value);
		}
		av[i]->timestamp = jav.timestamp;
		glite_jpis_AttrOrigToSoap(soap, jav.origin, &origin);
		av[i]->origin = *origin; free(origin);
		av[i]->originDetail = soap_strdup(soap, jav.origin_detail);		

		freeAttval_t(jav);
	} while (ret);
	
	glite_jp_db_freestmt(&stmt);	
	(*out).__sizeattributes = i;
	(*out).attributes = av;

	return 0;

err:
	glite_jp_db_freestmt(&stmt);	
	freeAttval_t(jav);
	return 1;
}


/* fills structure jobRecord */
static int get_attrs(struct soap *soap, glite_jpis_context_t isctx, char *jobid, struct _jpelem__QueryJobs *in, struct jptype__jobRecord **out)
{
	struct jptype__jobRecord 	jr;
	struct jptype__attrValue	**av;
	int 				j, size;


	assert(out);
	*out = soap_malloc(soap, sizeof(**out));
	memset(*out, 0, sizeof(**out));

	/* jobid */
	(*out)->jobid = soap_strdup(soap, jobid);

	/* sizeattributes & attributes */
	size = 0;
	for (j=0; in->__sizeattributes; j++) {
		if (get_attr(soap, isctx, jobid, in->attributes[j], &jr) ) goto err;
		av = realloc(av, (size + jr.__sizeattributes) * sizeof(*av));
		memcpy(av[size], jr.attributes, jr.__sizeattributes);
		size += jr.__sizeattributes;
		free(jr.attributes);
	} 
	(*out)->__sizeattributes = size;
	(*out)->attributes = soap_malloc( soap, size *sizeof(*((*out)->attributes)) );
	memcpy((*out)->attributes, av, size * sizeof(*((*out)->attributes)) );
	free(av);

	return 0;
	
err:
	return 1;
}

SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__QueryJobs(
	struct soap *soap,
	struct _jpelem__QueryJobs *in,
	struct _jpelem__QueryJobsResponse *out)
{
	CONTEXT_FROM_SOAP(soap, isctx);
//        glite_jp_context_t 		jpctx = isctx->jpctx;
//	glite_jp_query_rec_t		**qr;
	struct jptype__jobRecord	**jr;
	char				**jobids, **ps_list;
	int 				i, size;


	puts(__FUNCTION__);
//	if ( glite_jpis_SoapToQueryConds(soap, in->__sizeconditions, in->conditions, &qr) ) {
//		return SOAP_ERR;
//	}

	if ( checkIndexedConditions(isctx, in) ) {
		fprintf(stderr, "No indexed attribute in query\n");
		return SOAP_ERR;
	}

	if ( get_jobids(soap, isctx, in, &jobids, &ps_list) ) {
		return SOAP_ERR;
	}

	for (i=0; jobids[i]; i++);
	size = i;
	jr = soap_malloc(soap, size * sizeof(*jr));
	for (i=0; jobids[i]; i++) {
		if ( get_attrs(soap, isctx, jobids[i], in, &(jr[i])) ) {
			return SOAP_ERR;
		}	
		// XXX: in prototype we return only first value of PS URL
		// in future database shoul contain one more table with URLs
		jr[i]->__sizeprimaryStorage = 1;
		jr[i]->primaryStorage = soap_malloc(soap, sizeof(*(jr[i]->primaryStorage)));
		jr[i]->primaryStorage[0] = soap_strdup(soap, ps_list[i]);
		free(ps_list[i]);
		free(jobids[i]);
	}
	free(jobids);
	free(ps_list);

	(*out).__sizejobs = size;
	(*out).jobs = jr;
	
	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__AddFeed(
        struct soap *soap,
        struct _jpelem__AddFeed *in,
        struct _jpelem__AddFeedResponse *out)
{
        // XXX: test client in examples/jpis-test
        //      sends to this function some data for testing
        puts(__FUNCTION__);
        return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__GetFeedIDs(
        struct soap *soap,
        struct _jpelem__GetFeedIDs *in,
        struct _jpelem__GetFeedIDsResponse *out)
{
        // XXX: test client in examples/jpis-test
        //      sends to this function some data for testing
        puts(__FUNCTION__);
        return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__DeleteFeed(
        struct soap *soap,
        struct _jpelem__DeleteFeed *in,
        struct _jpelem__DeleteFeedResponse *out)
{
        // XXX: test client in examples/jpis-test
        //      sends to this function some data for testing
        puts(__FUNCTION__);
        return SOAP_OK;
}

