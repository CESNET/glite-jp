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


#define CONTEXT_FROM_SOAP(soap,ctx) glite_jpis_context_t	ctx = (glite_jpis_context_t) ((slave_data_t *) (soap->user))->ctx


static int updateJob(glite_jpis_context_t ctx, struct jptype__jobRecord *jobAttrs) {
	glite_jp_attrval_t av;
	struct jptype__attrValue *attr;
	int ret, iattrs;

	printf("%s: jobid='%s', attrs=%d\n", __FUNCTION__, jobAttrs->jobid, jobAttrs->__sizeattributes);

	if (jobAttrs->remove) assert(*(jobAttrs->remove) == 0);

	for (iattrs = 0; iattrs < jobAttrs->__sizeattributes; iattrs++) {
		attr = jobAttrs->attributes[iattrs];
		glite_jpis_SoapToAttrVal(&av, attr);
		if ((ret = glite_jpis_insertAttrVal(ctx, jobAttrs->jobid, &av)) != 0) return ret;
	}

	return 0;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__UpdateJobs(
	struct soap *soap,
	struct _jpelem__UpdateJobs *jpelem__UpdateJobs,
	struct _jpelem__UpdateJobsResponse *jpelem__UpdateJobsResponse)
{
	int 		ret, ijobs;
	const char 	*feedid;
	int 		status, done;
	CONTEXT_FROM_SOAP(soap, ctx);
	glite_jp_context_t jpctx = ctx->jpctx;

	// XXX: test client in examples/jpis-test
	//      sends to this function some data for testing
	puts(__FUNCTION__);

	// get info about the feed
	feedid = jpelem__UpdateJobs->feedId;
	memset(ctx->param_feedid, 0, sizeof(ctx->param_feedid));
	strncpy(ctx->param_feedid, feedid, sizeof(ctx->param_feedid) - 1);
	if ((ret = glite_jp_db_execute(ctx->select_info_feed_stmt)) != 1) {
		fprintf(stderr, "can't get info about '%s', returned %d records: %s (%s)\n", feedid, ret, jpctx->error->desc, jpctx->error->source);
		return SOAP_FAULT;
	}
	// update status, if needed (only oring)
	status = ctx->param_state;
	done = jpelem__UpdateJobs->feedDone ? GLITE_JP_IS_STATE_DONE : 0;
	if ((done != (status & GLITE_JP_IS_STATE_DONE)) && done) {
		ctx->param_state |= done;
		if ((ret = glite_jp_db_execute(ctx->update_state_feed_stmt)) != 1) {
			fprintf(stderr, "can't update state of '%s', returned %d records: %s (%s)\n", feedid, ret, jpctx->error->desc, jpctx->error->source);
			return SOAP_FAULT;
		}
	}

	// insert all attributes
	for (ijobs = 0; ijobs < jpelem__UpdateJobs->__sizejobAttributes; ijobs++) {
		if (updateJob(ctx, jpelem__UpdateJobs->jobAttributes[ijobs]) != 0) return SOAP_FAULT;
	}

	return SOAP_OK;
}


static int checkIndexedConditions(glite_jpis_context_t ctx, struct _jpelem__QueryJobs *in)
{
	char 			**indexed_attrs;
	int			i, j, k, ret;


	if ((ret = glite_jp_db_execute(ctx->select_info_attrs_indexed)) == -1) {
		fprintf(stderr, "Error when executing select_info_attrs_indexed, \
			returned %d records: %s (%s)\n", 
			ret, ctx->jpctx->error->desc, ctx->jpctx->error->source);
		return SOAP_FAULT;
	}

	i = 0;
	while (glite_jp_db_fetch(ctx->select_info_attrs_indexed) == 0) {
		if (!(i % INDEXED_STRIDE)) {
			indexed_attrs = realloc(indexed_attrs, 
				((i / INDEXED_STRIDE) * INDEXED_STRIDE + 1)  
				* sizeof(*indexed_attrs));
		}
		indexed_attrs[i++] = strdup(ctx->param_indexed);
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


/* adds attr table name to the list (null terminated) , iff unigue */
static void add_attr_table(char *new, char ***attr_tables) 
{
	int	i;
	
	for (i=0; (*attr_tables && *attr_tables[i]); i++) {
		if (!strcmp(*attr_tables[i], new)) return;
	}

//	*attr_tables = realloc((*attr_tables), (i+2) * sizeof(**attr_tables));
//	*attr_tables[i] = strdup(new);
//	*attr_tables[i+1] = NULL;
}	

/* transform soap enum queryOp to mysql quivalent */
static int get_op(const enum jptype__queryOp in, char **out)
{			
	char 			*qop;
	glite_jp_queryop_t      op;

	glite_jpis_SoapToQueryOp(in, &op);
	switch (op) {
		case GLITE_JP_QUERYOP_EQUAL:
			qop = strdup("=");
			break;
		case  GLITE_JP_QUERYOP_UNEQUAL:
			qop = strdup("!=");
			break;
		default:
			// unsupported query operator
			return(1);
			break;
	}

	*out = qop;
	return(0);
}

static int get_jobids(struct soap *soap, glite_jpis_context_t ctx, struct _jpelem__QueryJobs *in, char ***jobids, char *** ps_list)
{
	char 			*qa = NULL, *qb = NULL, *qop = NULL,
				*qwhere = NULL, *query = NULL, *res[2], 
				**jids = NULL, **pss = NULL, **attr_tables = NULL;
	int 			i, j, ret;
	glite_jp_db_stmt_t	stmt;
	glite_jp_attrval_t	attr;


	for (i=0; i < in->__sizeconditions; i++) {
		trio_asprintf(&qa,"%s jobs.jobid = attr_%|Ss.jobid AND (", 
			(i ? "AND" : ""), str2md5(in->conditions[i]->attr));
	
		for (j=0; j < in->conditions[i]->__sizerecord; j++) { 
			if (get_op(in->conditions[i]->record[j]->op, &qop)) goto err;
			add_attr_table(str2md5(in->conditions[i]->attr), &attr_tables);

			if (in->conditions[i]->record[j]->value->string) {
				attr.name = in->conditions[i]->attr;
				attr.value = in->conditions[i]->record[j]->value->string;
				attr.binary = 0;
				glite_jpis_SoapToAttrOrig(soap,
					in->conditions[i]->origin, &(attr.origin));
				trio_asprintf(&qb,"%s %s attr_%|Ss.value %s \"%|Ss\" ",
					qa, (j ? "OR" : ""), str2md5(in->conditions[i]->attr), qop,
					glite_jp_attrval_to_db_index(ctx->jpctx, &attr, 255));
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
				trio_asprintf(&qb,"%s %s attr_%|Ss.value %s \"%|Ss\" ",
					qa, (j ? "OR" : ""), str2md5(in->conditions[i]->attr), qop,
					glite_jp_attrval_to_db_index(ctx->jpctx, &attr, 255));
				free(qop);
				free(qa); qa = qb; qb = NULL; 
			}
		}
		trio_asprintf(&qb,"%s)",qa);
		free(qa); qa = qb; qb = NULL;
	}

	qwhere = qa; 
	qa = strdup("");

	for (i=0; (attr_tables && attr_tables[i]); i++) {
		trio_asprintf(&qb,"%s, %s", qa, attr_tables[i]);
		free(qa); qa = qb; qb = NULL;
	}
// XXX : memory destroyed somewhere (add_attr_table commented out too :(
	//trio_asprintf(&query, "SELECT dg_jobid FROM jobs%s WHERE %s;", qa, qwhere);
trio_asprintf(&query, "SELECT dg_jobid FROM jobs%s WHERE %s;", ",attr_d5189de027922f81005951e6efe0efd5", qwhere);
	free(qwhere);
	free(qa);
	
	// XXX: add where's for attr origin (not clear now whether stored in separate column
	// or obtained via glite_jp_attrval_from_db...

	if ((ret = glite_jp_db_execstmt(ctx->jpctx, query, &stmt)) < 0) goto err;
	free(query);

	i = 0;
	while ( (ret = glite_jp_db_fetchrow(stmt, res)) > 0 ) {
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
	} 

	if ( ret < 0 ) goto err;
	glite_jp_db_freestmt(&stmt);	

	*jobids = jids;
	*ps_list = pss;

	return 0;
	
err:
	free(query);
	for (i=0; (pss && pss[i]); i++) free(pss[i]);
	free(pss);
	for (i=0; (jids && jids[i]); i++) free(jids[i]);
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

static int get_attr(struct soap *soap, glite_jpis_context_t ctx, char *jobid, char *attr_name, struct jptype__jobRecord *out)
{
	glite_jp_attrval_t		jav;
	struct jptype__attrValue	**av;
	enum jptype__attrOrig		*origin;
	char 				*query, *fv;
	int 				i, ret;
	glite_jp_db_stmt_t      	stmt;


	trio_asprintf(&query,"SELECT full_value FROM attr_%|Ss WHERE jobid = %s",
		attr_name, jobid);

	if ((ret = glite_jp_db_execstmt(ctx->jpctx, query, &stmt)) < 0) goto err;
	free(query);

	i = 0;
	do {
		if ( (ret = glite_jp_db_fetchrow(stmt, &fv) < 0) ) goto err;
		i++;	
		av = realloc(av, i * sizeof(*av));
		av[i] = soap_malloc(soap, sizeof(*av[i]));

		if (glite_jp_attrval_from_db(ctx->jpctx, fv, &jav)) goto err;
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
static int get_attrs(struct soap *soap, glite_jpis_context_t ctx, char *jobid, struct _jpelem__QueryJobs *in, struct jptype__jobRecord **out)
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
		if (get_attr(soap, ctx, jobid, in->attributes[j], &jr) ) goto err;
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
	CONTEXT_FROM_SOAP(soap, ctx);
	struct jptype__jobRecord	**jr;
	char				**jobids, **ps_list;
	int 				i, size;


	puts(__FUNCTION__);
	memset(out, 0, sizeof(*out));

	if ( checkIndexedConditions(ctx, in) ) {
		fprintf(stderr, "No indexed attribute in query\n");
		return SOAP_ERR;
	}

	if ( get_jobids(soap, ctx, in, &jobids, &ps_list) ) {
		return SOAP_ERR;
	}

	for (i=0; (jobids && jobids[i]); i++);
	size = i;
	jr = soap_malloc(soap, size * sizeof(*jr));
	for (i=0; (jobids && jobids[i]); i++) {
		if ( get_attrs(soap, ctx, jobids[i], in, &(jr[i])) ) {
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

