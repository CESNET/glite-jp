#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/strmd5.h"
#include "glite/jp/attr.h"
#include "glite/jp/known_attr.h"
#include "glite/lb/trio.h"

#include "jp_H.h"
#include "soap_version.h"
#include "glite/security/glite_gscompat.h"
#include "db_ops.h"
#include "ws_ps_typeref.h"
#include "ws_is_typeref.h"
#include "context.h"
#include "common.h"

#define	INDEXED_STRIDE	2	// how often realloc indexed attr result
				// XXX: 2 is only for debugging, replace with e.g. 100
#define	JOBIDS_STRIDE	2	// how often realloc matched jobids result

/*------------------*/
/* Helper functions */
/*------------------*/

#include "glite/jp/ws_fault.c"



/*-----------------------------------------*/
/* IS WSDL server function implementations */
/*-----------------------------------------*/


#define CONTEXT_FROM_SOAP(soap,ctx) glite_jpis_context_t	ctx = (glite_jpis_context_t) ((slave_data_t *) (soap->user))->ctx


static int updateJob(glite_jpis_context_t ctx, const char *ps, struct jptype__jobRecord *jobAttrs) {
	glite_jp_attrval_t av;
	struct jptype__attrValue *attr;
	int ret, iattrs;

	lprintf("jobid='%s', attrs=%d\n", jobAttrs->jobid, jobAttrs->__sizeattributes);

	if (jobAttrs->remove) assert(*(jobAttrs->remove) == GLITE_SECURITY_GSOAP_FALSE);

	if ((ret = glite_jpis_lazyInsertJob(ctx, ps, jobAttrs->jobid, jobAttrs->owner)) != 0) return ret;
	for (iattrs = 0; iattrs < jobAttrs->__sizeattributes; iattrs++) {
		attr = GLITE_SECURITY_GSOAP_LIST_GET(jobAttrs->attributes, iattrs);
		glite_jpis_SoapToAttrVal(&av, attr);
		if ((ret = glite_jpis_insertAttrVal(ctx, jobAttrs->jobid, &av)) != 0) return ret;
	}

	return 0;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__UpdateJobs(
	struct soap *soap,
	struct _jpelem__UpdateJobs *jpelem__UpdateJobs,
	struct _jpelem__UpdateJobsResponse *jpelem__UpdateJobsResponse UNUSED)
{
	int 		ret, ijobs;
	const char 	*feedid;
	int 		status, done;
	CONTEXT_FROM_SOAP(soap, ctx);
	glite_jp_context_t jpctx = ctx->jpctx;
	char *err, *ps;

	// XXX: test client in examples/jpis-test
	//      sends to this function some data for testing
	puts(__FUNCTION__);
	ps = NULL;

	// get info about the feed
	feedid = jpelem__UpdateJobs->feedId;
	lprintf("feedid='%s'\n", feedid);

	GLITE_JPIS_PARAM(ctx->param_feedid, ctx->param_feedid_len, feedid);
	if ((ret = glite_jp_db_execute(ctx->select_info_feed_stmt)) != 1) {
		fprintf(stderr, "can't get info about feed '%s', returned %d records", feedid, ret);
		if (jpctx->error) fprintf(stderr, ": %s (%s)\n", jpctx->error->desc, jpctx->error->source);
		else fprintf(stderr, "\n");
		goto fail;
	}
	if (glite_jp_db_fetch(ctx->select_info_feed_stmt) != 0) {
		fprintf(stderr, "can't fetch feed '%s'", feedid);
		if (jpctx->error) fprintf(stderr, ": %s (%s)\n", jpctx->error->desc, jpctx->error->source);
		else fprintf(stderr, "\n");
		goto fail;
	}
	lprintf("uniqueid=%ld, state=%d, source='%s'\n", ctx->param_uniqueid, ctx->param_state, ctx->param_ps);
	ps = strdup(ctx->param_ps);

	// update status, if needed (only orig)
	status = ctx->param_state;
	done = jpelem__UpdateJobs->feedDone ? GLITE_JP_IS_STATE_DONE : 0;
	if ((done != (status & GLITE_JP_IS_STATE_DONE)) && done) {
		ctx->param_state |= done;
		if ((ret = glite_jp_db_execute(ctx->update_state_feed_stmt)) != 1) {
			fprintf(stderr, "can't update state of '%s', returned %d records", feedid, ret);
			if (jpctx->error) fprintf(stderr, ": %s (%s)\n", jpctx->error->desc, jpctx->error->source);
			else fprintf(stderr, "\n");
			goto fail;
		}
	}

	// insert all attributes
	for (ijobs = 0; ijobs < jpelem__UpdateJobs->__sizejobAttributes; ijobs++) {
		if (updateJob(ctx, (const char *) ps, GLITE_SECURITY_GSOAP_LIST_GET(jpelem__UpdateJobs->jobAttributes, ijobs)) != 0) goto fail;
	}
	free(ps);

	return SOAP_OK;

fail:
	free(ps);
	if (ctx->jpctx->error) {
		err = glite_jp_error_chain(ctx->jpctx);
		fprintf(stderr, "%s:%s\n", __FUNCTION__, err);
		free(err);
	}
	glite_jp_server_err2fault(ctx->jpctx, soap);
	return SOAP_FAULT;
}


static int checkIndexedConditions(glite_jpis_context_t ctx, struct _jpelem__QueryJobs *in)
{
	char 			**indexed_attrs = NULL, *res;
	int			i, j, k, ret;
	glite_jp_db_stmt_t      stmt;


	if ((ret = glite_jp_db_execstmt(ctx->jpctx, 
		"SELECT name FROM attrs WHERE (indexed=1)", &stmt)) < 0) goto end;
	
	i = 0;
        while ( (ret = glite_jp_db_fetchrow(stmt, &res)) > 0 ) {
		if (!(i % INDEXED_STRIDE)) {
			indexed_attrs = realloc(indexed_attrs, 
				((i / INDEXED_STRIDE + 1) * INDEXED_STRIDE)  
				* sizeof(*indexed_attrs));
		}
		indexed_attrs[i++] = strdup(res);
		free(res);
        }
        if ( ret < 0 ) goto end;

	for (k=0; k < in->__sizeconditions; k++) {
		for (j=0; j < i; j++) {
			char *attr = GLITE_SECURITY_GSOAP_LIST_GET(in->conditions, k)->attr;
			if (!strcmp(attr, GLITE_JP_ATTR_JOBID) || !strcmp(attr, indexed_attrs[j])) {
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


static int checkConditions(glite_jpis_context_t ctx, struct _jpelem__QueryJobs *in) {
	int i, j;
	char *attr;

	for (i = 0; i < in->__sizeconditions; i++) {
		attr = GLITE_SECURITY_GSOAP_LIST_GET(in->conditions, i)->attr;
		if (!attr) return 1;
		for (j = 0; ctx->conf->attrs[j] && strcasecmp(ctx->conf->attrs[j], attr) != 0; j++);
		if (!ctx->conf->attrs[j]) return 1;
	}

	return 0;
}


/* adds attr table name to the list (null terminated) , iff unigue */
static void add_attr_table(char *new, char ***attr_tables) 
{
	int	i;
	
	for (i=0; (*attr_tables && (*attr_tables)[i]); i++) {
		if (!strcmp((*attr_tables)[i], new)) return;
	}

	*attr_tables = realloc((*attr_tables), (i+2) * sizeof(**attr_tables));
	(*attr_tables)[i] = strdup(new);
	(*attr_tables)[i+1] = NULL;
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
		case  GLITE_JP_QUERYOP_GREATER:
			qop = strdup(">");
			break;
		case  GLITE_JP_QUERYOP_LESS:
			qop = strdup("<");
			break;
		case  GLITE_JP_QUERYOP_WITHIN:
			qop = strdup("BETWEEN");
			break;
		default:
			// unsupported query operator
			return(1);
			break;
	}

	*out = qop;
	return(0);
}


static char *get_sql_stringvalue(struct jptype__stringOrBlob *value) {
	if (!value) return NULL;
	if (!GSOAP_ISSTRING(value)) return NULL;
	return GSOAP_STRING(value);
}


static int get_sql_indexvalue(char **sql, glite_jpis_context_t ctx, struct jptype__indexQuery *condition, struct jptype__stringOrBlob *value) {
	glite_jp_attrval_t attr;

	*sql = NULL;
	if (!value) return 0;
	memset(&attr, 0, sizeof attr);
	attr.name = condition->attr;
	if (GSOAP_ISSTRING(value)) {
		attr.value = GSOAP_STRING(value);
		attr.binary = 0;
	} else if (GSOAP_ISBLOB(value)) {
		attr.value = GSOAP_BLOB(value)->__ptr;
		attr.size = GSOAP_BLOB(value)->__size;
		attr.binary = 1;
	} else return 0;
	glite_jpis_SoapToAttrOrig(condition->origin, &(attr.origin));

	*sql = glite_jp_attrval_to_db_index(ctx->jpctx, &attr, 255);
	return 0;
}


static int get_sql_cond(char **sql, const char *attr_md5, enum jptype__queryOp op, char *value, char *value2) {
	char *s, *qop, *column;

	*sql = NULL;
	if (get_op(op, &qop) != 0) return 0;
	if (attr_md5) trio_asprintf(&column, "attr_%|Ss.value", attr_md5);
	else asprintf(&column, "jobs.dg_jobid");
	trio_asprintf(sql, "%s %s '%|Ss'", column, qop, value);
	free(column);
	free(qop);
	if (op == jptype__queryOp__WITHIN) {
		if (!value) {
			free(*sql);
			*sql = NULL;
			return EINVAL;
		}
		trio_asprintf(&s, "%s AND '%|Ss'", *sql, value2);
		free(*sql); *sql = s;
	}
	return 0;
}


static char *get_sql_or(glite_jpis_context_t ctx, struct jptype__indexQuery *condition, const char *attr_md5) {
	struct jptype__indexQueryRecord *record;
	char *sql, *cond, *s = NULL, *value, *value2;
	int j;

	sql = strdup("");
	for (j=0; j < condition->__sizerecord; j++) { 
		record = GLITE_SECURITY_GSOAP_LIST_GET(condition->record, j);
		if (record->op == jptype__queryOp__EXISTS) {
			/* no additional conditions needed when existing is enough */
		} else {
			if (strcmp(condition->attr, GLITE_JP_ATTR_JOBID) == 0) {
				value = get_sql_stringvalue(record->value);
				if (!value) goto err;
				value2 = get_sql_stringvalue(record->value2);
				if (get_sql_cond(&cond, attr_md5, record->op, value, value2) != 0) goto err;
			} else {
				get_sql_indexvalue(&value, ctx, condition, record->value);
				get_sql_indexvalue(&value2, ctx, condition, record->value2);
				get_sql_cond(&cond, attr_md5, record->op, value, value2);
				free(value);
				free(value2);
				if (!cond) goto err;
			}
			trio_asprintf(&s,"%s%s%s", sql, (sql[0] ? " OR " : ""), cond);
			free(cond);
			free(sql); sql = s;
		}
	}

	return sql;
err:
	free(sql);
	free(s);
	return NULL;
}


/* get all jobids matching the query conditions */
static int get_jobids(glite_jpis_context_t ctx, struct _jpelem__QueryJobs *in, char ***jobids, char *** ps_list)
{
	char 			*qa = NULL, *qb = NULL, *qor, *attr_md5,
				*qwhere = NULL, *query = NULL, *res[2], 
				**jids = NULL, **pss = NULL, **attr_tables = NULL;
	int 			i, ret;
	glite_jp_db_stmt_t	stmt = NULL;
	glite_jp_attr_orig_t	orig;

	
	qwhere = strdup("");
	for (i=0; i < in->__sizeconditions; i++) {
		struct jptype__indexQuery *condition;

		condition = GLITE_SECURITY_GSOAP_LIST_GET(in->conditions, i);

		/* attr name */
		if (strcmp(condition->attr, GLITE_JP_ATTR_JOBID) == 0) {
			/* no subset from attr_ table, used jobs table instead */
			attr_md5 = NULL;
			qa = strdup("");
		} else {
			attr_md5 = str2md5(condition->attr);
			add_attr_table(attr_md5, &attr_tables);

			/* origin */
			if (condition->origin) {
				glite_jpis_SoapToAttrOrig(condition->origin, &orig);
				trio_asprintf(&qb, "attr_%|Ss.origin = %d AND ", attr_md5, orig);
			} else
				trio_asprintf(&qb, "");

			/* select given records in attr_ table */
			trio_asprintf(&qa,"%s%sjobs.jobid = attr_%|Ss.jobid",
				(i ? "AND " : ""), qb, attr_md5);

			free(qb);
		}

		/* inside part of the condition: record list (ORs) */
		if ((qor = get_sql_or(ctx, condition, attr_md5)) == NULL) goto err;
		if (qor[0]) {
			asprintf(&qb, "%s%s(%s)", qa, qa[0] ? " AND " : "", qor);
			free(qa);
			qa = qb;
		}
		free(qor);

		trio_asprintf(&qb,"%s%s%s", qwhere, qa[0] ? " " : "", qa);
		free(qa); qwhere = qb; qb = NULL; qa = NULL;
		free(attr_md5);
	}

	qa = strdup("");

	for (i=0; (attr_tables && attr_tables[i]); i++) {
		trio_asprintf(&qb,"%s, attr_%s", qa, attr_tables[i]);
		free(qa); qa = qb; qb = NULL;
	}

	if (ctx->conf->no_auth) {
		trio_asprintf(&query, "SELECT DISTINCT dg_jobid,ps FROM jobs%s WHERE%s", qa, qwhere);
	}
	else {
		trio_asprintf(&query, "SELECT DISTINCT dg_jobid,ps FROM jobs,users%s WHERE (jobs.ownerid = users.userid AND users.cert_subj='%s') AND%s", qa, ctx->jpctx->peer, qwhere);
	}
	printf("Incomming QUERY:\n %s\n", query);
	free(qwhere);
	free(qa);
	
	if ((ret = glite_jp_db_execstmt(ctx->jpctx, query, &stmt)) < 0) goto err;
	free(query);

	i = 0;
	while ( (ret = glite_jp_db_fetchrow(stmt, res)) > 0 ) {
		if (!(i % JOBIDS_STRIDE)) {
                        jids = realloc(jids,
                                ((i / JOBIDS_STRIDE + 1) * JOBIDS_STRIDE + 1)
                                * sizeof(*jids));
                }
		if (!(i % JOBIDS_STRIDE)) {
                        pss = realloc(pss,
                                ((i / JOBIDS_STRIDE + 1) * JOBIDS_STRIDE + 1)
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
/* Needs to be copied to list using soap_malloc in calling function!	*/

static int get_attr(struct soap *soap, glite_jpis_context_t ctx, char *jobid, char *attr_name, int *size, struct jptype__attrValue **out)
{
	glite_jp_attrval_t		jav;
	struct jptype__attrValue	*av;
	enum jptype__attrOrig		*origin;
	char 				*query, *fv, *jobid_md5, *attr_md5;
	int 				i, ret;
	glite_jp_db_stmt_t      	stmt;

	memset(&jav,0,sizeof(jav));
	jobid_md5 = str2md5(jobid);
	attr_md5 = str2md5(attr_name);
	trio_asprintf(&query,"SELECT full_value FROM attr_%|Ss WHERE jobid = \"%s\"",
		attr_md5, jobid_md5);
	free(attr_md5);
	free(jobid_md5);

	if ((ret = glite_jp_db_execstmt(ctx->jpctx, query, &stmt)) < 0) {
		glite_jpis_stack_error(ctx->jpctx, EIO, "SELECT from attribute '%s' failed", attr_name);
		goto err;
	}
	free(query);

	av = *out;
	i = *size;
	while ( (ret = glite_jp_db_fetchrow(stmt, &fv)) > 0 ) {	
		av = realloc(av, (i+1) * sizeof(*av));
		memset(&av[i], 0, sizeof(av[i]));

		memset(&jav,0,sizeof(jav));
		if (glite_jp_attrval_from_db(ctx->jpctx, fv, &jav)) goto err;
		av[i].name = soap_strdup(soap, attr_name);
		av[i].value = soap_malloc(soap, sizeof(*(av[i].value)));
		memset(av[i].value, 0, sizeof(*(av[i].value)));
		if (jav.binary) {
			GSOAP_SETBLOB(av[i].value, soap_malloc(soap, sizeof(*(GSOAP_BLOB(av[i].value)))));
			memset(GSOAP_BLOB(av[i].value), 0, sizeof(*(GSOAP_BLOB(av[i].value))));
			GSOAP_BLOB(av[i].value)->__ptr = soap_malloc(soap, jav.size);
			memcpy(GSOAP_BLOB(av[i].value)->__ptr, jav.value, jav.size);
			GSOAP_BLOB(av[i].value)->__size = jav.size;
			// XXX: id, type, option - how to handle?
		}
		else {
			GSOAP_SETSTRING(av[i].value, jav.value ? soap_strdup(soap, jav.value) : NULL);
		}
		av[i].timestamp = jav.timestamp;
		glite_jpis_AttrOrigToSoap(soap, jav.origin, &origin);
		// atribute has always origin
		assert(origin != GLITE_JP_ATTR_ORIG_ANY);
		av[i].origin = *origin; soap_dealloc(soap, origin);
		if (jav.origin_detail) av[i].originDetail = soap_strdup(soap, jav.origin_detail);		

		i++;
		freeAttval_t(jav);
	} 
	if (ret < 0) goto err;
	
	glite_jp_db_freestmt(&stmt);
	*size = i;
	*out = av;

	return 0;

err:
	glite_jp_db_freestmt(&stmt);	
	freeAttval_t(jav);
	return 1;
}


/* return owner of job record */
static int get_owner(glite_jpis_context_t ctx, char *jobid, char **owner)
{
	char			*ownerid = NULL, *jobid_md5, *query, *fv = NULL;
	glite_jp_db_stmt_t     	stmt;


	/* get ownerid correspondig to jobid */
	jobid_md5 = str2md5(jobid);
	trio_asprintf(&query,"SELECT ownerid FROM jobs WHERE jobid = \"%s\"",
                jobid_md5);
        free(jobid_md5);

	if ((glite_jp_db_execstmt(ctx->jpctx, query, &stmt)) < 0) goto err;
        free(query);

        if (glite_jp_db_fetchrow(stmt, &ownerid) <= 0 ) goto err; 
	
	/* DB consistency check - only one record per jobid ! */
	assert (glite_jp_db_fetchrow(stmt, &fv) <=0); free(fv);
	

	/* get cert_subj corresponding to ownerid */
	trio_asprintf(&query,"SELECT cert_subj FROM users WHERE userid = \"%s\"",
		ownerid);

	if ((glite_jp_db_execstmt(ctx->jpctx, query, &stmt)) < 0) goto err;
        free(query);

	if (glite_jp_db_fetchrow(stmt, owner) <= 0 ) goto err;

        /* DB consistency check - only one record per userid ! */
        assert (glite_jp_db_fetchrow(stmt, &fv) <=0); free(fv);


	return 0;
err:
	free(ownerid);
	free(query);
	return 1;
}

/* fills structure jobRecord  for a given jobid*/
static int get_attrs(struct soap *soap, glite_jpis_context_t ctx, char *jobid, struct _jpelem__QueryJobs *in, struct jptype__jobRecord *out)
{
	struct jptype__attrValue	*av = NULL;
	int 				j, size = 0;


	assert(out);
	memset(out, 0, sizeof(*out));

	/* jobid */
	out->jobid = soap_strdup(soap, jobid);

	/* sizeattributes & attributes */
	size = 0;
	for (j=0; j < in->__sizeattributes; j++)
		if (get_attr(soap, ctx, jobid, in->attributes[j], &size, &av) ) goto err;
	if ( get_owner(ctx, jobid, &(out->owner)) ) goto err;

	GLITE_SECURITY_GSOAP_LIST_CREATE(soap, out, attributes, struct jptype__attrValue, size);
	for (j = 0; j < size; j++)
		memcpy(GLITE_SECURITY_GSOAP_LIST_GET(out->attributes, j), &av[j], sizeof(av[0]));
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
	struct jptype__jobRecord	*jr;

	char				**jobids = NULL, **ps_list = NULL;
	int 				i, size, err;


	puts(__FUNCTION__);
	memset(out, 0, sizeof(*out));
	
	/* test whether there is any indexed attribudes in the condition */
	if ( checkIndexedConditions(ctx, in) ) {
		glite_jpis_stack_error(ctx->jpctx, EINVAL, "No indexed attribute in query");
		goto fail;
	}

	/* test whether there is known attribudes in the condition */
	if ( checkConditions(ctx, in) ) {
		glite_jpis_stack_error(ctx->jpctx, EINVAL, "Unknown attribute in query");
		goto fail;
	}

	/* get all jobids matching the conditions */
	if ( (err = get_jobids(ctx, in, &jobids, &ps_list)) != 0 ) {
		glite_jpis_stack_error(ctx->jpctx, err, "Error getting jobs");
		goto fail;
	}

	/* get all requested attributes for matching jobids */
	for (i=0; (jobids && jobids[i]); i++);
	size = i;
	GLITE_SECURITY_GSOAP_LIST_CREATE(soap, out, jobs, struct jptype__jobRecord, size);
	for (i=0; (jobids && jobids[i]); i++) {
		jr = GLITE_SECURITY_GSOAP_LIST_GET(out->jobs, i);
		if ( (err = get_attrs(soap, ctx, jobids[i], in, jr)) != 0 ) {
			glite_jpis_stack_error(ctx->jpctx, err, "Error getting attributes of the job '%s'", jobids[i]);
			goto fail;
		}

		// XXX: in prototype we return only first value of PS URL
		// in future database should contain one more table with URLs
		jr->__sizeprimaryStorage = 1;
		jr->primaryStorage = soap_malloc(soap, sizeof(*(jr->primaryStorage)));
		jr->primaryStorage[0] = soap_strdup(soap, ps_list[i]);
		free(ps_list[i]);
		free(jobids[i]);
	}
	free(jobids);
	free(ps_list);

	return SOAP_OK;
fail:
	glite_jp_server_err2fault(ctx->jpctx, soap);
	return SOAP_ERR;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__AddFeed(
        struct soap *soap UNUSED,
        struct _jpelem__AddFeed *in UNUSED,
        struct _jpelem__AddFeedResponse *out UNUSED)
{
        // XXX: test client in examples/jpis-test
        //      sends to this function some data for testing
        puts(__FUNCTION__);
        return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__GetFeedIDs(
        struct soap *soap UNUSED,
        struct _jpelem__GetFeedIDs *in UNUSED,
        struct _jpelem__GetFeedIDsResponse *out UNUSED)
{
        // XXX: test client in examples/jpis-test
        //      sends to this function some data for testing
        puts(__FUNCTION__);
        return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__DeleteFeed(
        struct soap *soap UNUSED,
        struct _jpelem__DeleteFeed *in UNUSED,
        struct _jpelem__DeleteFeedResponse *out UNUSED)
{
        // XXX: test client in examples/jpis-test
        //      sends to this function some data for testing
        puts(__FUNCTION__);
        return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__ServerConfiguration(
        struct soap *soap UNUSED,
        struct _jpelem__ServerConfiguration *in UNUSED,
        struct _jpelem__ServerConfigurationResponse *out UNUSED)
{
	// empty, just for deserializer generation
        puts(__FUNCTION__);
        return SOAP_OK;
}

