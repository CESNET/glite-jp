#ident "$Header$"

#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <glite/lbu/trio.h>
#include <glite/jobid/strmd5.h>
#include <glite/jobid/cjobid.h>
#include <glite/jp/types.h>
#include <glite/jp/context.h>
#include <glite/jp/db.h>
#include <glite/jp/attr.h>
#include "glite/jp/known_attr.h"

#include "conf.h"
#include "context.h"
#include "db_ops.h"
#include "common.h"


#ifndef LOG_SQL
#define LOG_SQL 1
#endif

#define TABLE_PREFIX_DATA "attr_"
#define SQLCMD_DROP_DATA_TABLE "DROP TABLE " TABLE_PREFIX_DATA "%s"
#define SQLCMD_CREATE_DATA_TABLE "CREATE TABLE " TABLE_PREFIX_DATA "%s (\n\
        `jobid`          CHAR(32)    BINARY NOT NULL,\n\
        `value`          %s          BINARY NOT NULL,\n\
        `full_value`     %s          NOT NULL,\n\
        `origin`         INT         NOT NULL,\n\
\n\
        INDEX (jobid),\n\
        INDEX (value)\n\
) ENGINE=innodb;"
#define SQLCMD_INSERT_ATTRVAL "INSERT INTO " TABLE_PREFIX_DATA "%|Ss (jobid, value, full_value, origin) VALUES (\n\
	'%|Ss',\n\
	'%|Ss',\n\
	'%|Ss',\n\
	'%ld'\n\
)"
#define INDEX_LENGTH 255

#define WORD_SWAP(X) ((((X) >> 8) & 0xFF) | (((X) & 0xFF) << 8))
#define LONG_SWAP(X) (WORD_SWAP(((X) >> 16) & 0xFFFF) | ((WORD_SWAP(X) & 0xFFFF) << 16))
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define LONG_LE(X) (X)
#else
#define LONG_LE(X) LONG_SWAP(X)
#endif

#define COND_MAGIC 0x444E4F43


static int glite_jpis_db_queries_deserialize(glite_jp_query_rec_t **queries, void *blob, size_t blob_size) UNUSED;


static int is_indexed(glite_jp_is_conf *conf, const char *attr) {
	size_t i;

	i = 0;
	while (conf->indexed_attrs[i]) {
		if (strcasecmp(attr, conf->indexed_attrs[i]) == 0) return 1;
		i++;
	}
	return 0;
}


static size_t db_arg1_length(glite_jpis_context_t isctx, glite_jp_query_rec_t *query) {
	size_t len;

	assert(query->op > GLITE_JP_QUERYOP_UNDEF && query->op <= GLITE_JP_QUERYOP__LAST);
	if (isctx->op_args[query->op] >= 1)
		len = query->binary ? query->size : (query->value ? strlen(query->value) + 1 : 0);
	else len = 0;

	return len;
}

static size_t db_arg2_length(glite_jpis_context_t isctx, glite_jp_query_rec_t *query) {
	size_t len;

	assert(query->op > GLITE_JP_QUERYOP_UNDEF && query->op <= GLITE_JP_QUERYOP__LAST);
	if (isctx->op_args[query->op] >= 1)
		len = query->binary ? query->size2 : (query->value2 ? strlen(query->value2) + 1 : 0);
	else len = 0;

	return len;
}


static int array_init(void **data, size_t *len, size_t *maxlen, size_t initial_len) {
	*len = 0;
	if ((*data = malloc(initial_len)) != NULL) {
		*maxlen = initial_len;
		return 0;
	} else {
		*maxlen = 0;
		return ENOMEM;
	}
}


static int array_add(void **data, size_t *len, size_t *maxlen, void *new_data, size_t new_data_len) {
	void *tmp;
	size_t ptr;

	ptr = *len;
	(*len) += new_data_len;
	if (*len > *maxlen) {
		do {
			(*maxlen) *= 2;
		} while (*len > *maxlen);
		if ((tmp = realloc(*data, *maxlen)) == NULL) return ENOMEM;
		*data = tmp;
	}
	memcpy(((char *)(*data)) + ptr, new_data, new_data_len);

	return 0;
}


static int array_add_long(void **data, size_t *len, size_t *maxlen, uint32_t l) {
	uint32_t lel;

	lel = LONG_LE(l);
	return array_add(data, len, maxlen, &lel, sizeof(uint32_t));
}


static uint32_t array_get_long(void **data) {
	uint32_t *lel;

	lel = (uint32_t *)*data;
	*data = ((char *)*data) + sizeof(uint32_t);

	return LONG_LE(*lel);
}


static void *array_get(void **data, size_t data_len) {
	void *res;

	res = *data;
	*data = ((char *)*data) + data_len;

	return res;
}


static int glite_jpis_db_queries_serialize(glite_jpis_context_t isctx, void **blob, size_t *len, glite_jp_query_rec_t *queries) {
	size_t maxlen;
	glite_jp_query_rec_t *query;
	int ret;
	size_t datalen;

	if ((ret = array_init(blob, len, &maxlen, 1024)) != 0) return ret;
	query = queries;
	while(query && query->attr) {
		if ((ret = array_add_long(blob, len, &maxlen, COND_MAGIC)) != 0) goto fail;
		datalen = strlen(query->attr) + 1;
		if ((ret = array_add_long(blob, len, &maxlen, datalen)) != 0) goto fail;
		if ((ret = array_add(blob, len, &maxlen, query->attr, datalen)) != 0) goto fail;
		if ((ret = array_add_long(blob, len, &maxlen, query->op)) != 0) goto fail;
		if ((ret = array_add_long(blob, len, &maxlen, query->binary ? 1 : 0)) != 0) goto fail;

		datalen = db_arg1_length(isctx, query);
		if ((ret = array_add_long(blob, len, &maxlen, datalen)) != 0) goto fail;
		if (datalen)
			if ((ret = array_add(blob, len, &maxlen, query->value, datalen)) != 0) goto fail;
		
		datalen = db_arg2_length(isctx, query);
		if ((ret = array_add_long(blob, len, &maxlen, datalen)) != 0) goto fail;
		if (datalen)
			if ((ret = array_add(blob, len, &maxlen, query->value2, datalen)) != 0) goto fail;

		query++;
	}

	return 0;
fail:
	free(*blob);
	*len = 0;
	return ret;
}


static int glite_jpis_db_queries_deserialize(glite_jp_query_rec_t **queries, void *blob, size_t blob_size) {
	size_t maxlen, len, datalen;
	void *blob_ptr, *blob_end;
	int ret;
	uint32_t l;
	glite_jp_query_rec_t query;
	int i;

	if ((ret = array_init((void *)queries, &len, &maxlen, 512)) != 0) return ret;
	blob_ptr = blob;
	blob_end = (char *)blob + blob_size;
	while (blob_end > blob_ptr) {
		ret = ENOMEM;
		memset(&query, 0, sizeof query);
		l = array_get_long(&blob_ptr);
		if (l != COND_MAGIC) {
			lprintf("blob=%p, blob_ptr=%p, 0x%08" PRIX32 "\n", blob, blob_ptr, l);
			ret = EINVAL;
			goto fail_query;
		}

		datalen = array_get_long(&blob_ptr);
		if (datalen) {
			if ((query.attr = malloc(datalen)) == NULL) goto fail_query;
			memcpy(query.attr, array_get(&blob_ptr, datalen), datalen);
		} else query.attr = NULL;

		query.op = array_get_long(&blob_ptr);
		query.binary = array_get_long(&blob_ptr);

		datalen = array_get_long(&blob_ptr);
		if (datalen) {
			if ((query.value = malloc(datalen)) == NULL) goto fail_query;
			memcpy(query.value, array_get(&blob_ptr, datalen), datalen);
		} else query.value = NULL;
		query.size = datalen;

		datalen = array_get_long(&blob_ptr);
		if (datalen) {
			if ((query.value2 = malloc(datalen)) == NULL) goto fail_query;
			memcpy(query.value2, array_get(&blob_ptr, datalen), datalen);
		} else query.value2 = NULL;
		query.size2 = datalen;

		if ((ret = array_add((void *)queries, &len, &maxlen, &query, sizeof(query))) != 0) goto fail_query;
	}
	assert(blob_end == blob_ptr);

	memset(&query, 0, sizeof query);
	if ((ret = array_add((void *)queries, &len, &maxlen, &query, sizeof(query))) != 0) goto fail;

	return 0;

fail_query:
fail:
	i = 0;
	while ((*queries)[i].attr) {
		free((*queries)[i].attr);
		free((*queries)[i].value);
		free((*queries)[i].value2);
		i++;
	}
	free(*queries);
	return ret;
}


/**
 * Convert attribute name to attribute id.
 */
char *glite_jpis_attr_name2id(const char *name) {
	size_t i, len;
	char *lname, *id;

	len = strlen(name);
	lname = malloc(len + 1);
	for (i = 0; i < len + 1; i++) lname[i] = tolower(name[i]);
	id = str2md5(lname);
	free(lname);

	return id;
//	return str2md5(name);
}


/* Init the database. 
 *
 * \retval 0        OK
 * \retval non-zero JP error code
 */

int glite_jpis_initDatabase(glite_jpis_context_t ctx) {
	char **attrs, *attrid, *num;
	const char *type_index, *type_full;
	size_t i;
	int indexed, state, locked, nattrs;
	size_t conds_len;
	char sql[512];
	glite_jp_is_feed **feeds;
	void *conds;
	glite_jp_context_t jpctx = ctx->jpctx;
	glite_lbu_Statement stmt = NULL;

	jpctx = ctx->jpctx;

	// check, if database was already created
	if (glite_jp_db_ExecSQL(jpctx, "SELECT COUNT(*) FROM attrs", &stmt) < 0) {
		glite_jpis_stack_error(ctx->jpctx, EIO, "error during counting attrs");
		goto fail;
	}
	if (glite_jp_db_FetchRow(jpctx, stmt, 1, NULL, &num) < 0) {
		glite_jpis_stack_error(ctx->jpctx, EIO, "error during fetching attrs");
		goto fail;
	}
	nattrs = atoi(num);
	llprintf(LOG_SQL, "found '%s' attributes in attrs table\n", num, nattrs);
	free(num);
	glite_jp_db_FreeStmt(&stmt);
	if (nattrs != 0) {
		lprintf("database with %d attributes kept (use -D for delete)\n", nattrs);
		return 0;
	}

	if (glite_jp_db_PrepareStmt(jpctx, "INSERT INTO attrs (attrid, name, indexed, type) VALUES (?, ?, ?, ?)", &stmt) != 0) {
		glite_jpis_stack_error(ctx->jpctx, EIO, "can't create insert attributes statement");
		goto fail;
	}

	// attrs table and attrid_* tables
	attrs = ctx->conf->attrs;
	i = 0;
	if (attrs) while (attrs[i]) {
		type_full = glite_jp_attrval_db_type_full(jpctx, attrs[i]);
		type_index = glite_jp_attrval_db_type_index(jpctx, attrs[i], INDEX_LENGTH);

		attrid = glite_jpis_attr_name2id(attrs[i]);
		indexed = is_indexed(ctx->conf, attrs[i]);
		if (glite_jp_db_ExecPreparedStmt(jpctx, stmt, 4,
		  GLITE_LBU_DB_TYPE_VARCHAR, attrid,
		  GLITE_LBU_DB_TYPE_VARCHAR, attrs[i],
		  GLITE_LBU_DB_TYPE_INT, indexed,
		  GLITE_LBU_DB_TYPE_VARCHAR, type_full) == -1) {
			glite_jpis_stack_error(ctx->jpctx, EIO, "can't create '%s' attribute", attrs[i]);
			goto fail;
		}

		// silently drop
		sql[sizeof(sql) - 1] = '\0';
		snprintf(sql, sizeof(sql), SQLCMD_DROP_DATA_TABLE, attrid);
		llprintf(LOG_SQL, "preventive dropping '%s' ==> '%s'\n", attrid, sql);
		glite_jp_db_ExecSQL(jpctx, sql, NULL);
		glite_jp_clear_error(ctx->jpctx);

		// create table
		sql[sizeof(sql) - 1] = '\0';
		snprintf(sql, sizeof(sql) - 1, SQLCMD_CREATE_DATA_TABLE, attrid, type_index, type_full);
		free(attrid);
		llprintf(LOG_SQL, "creating table: '%s'\n", sql);
		if ((glite_jp_db_ExecSQL(jpctx, sql, NULL)) == -1) {
			glite_jpis_stack_error(ctx->jpctx, EAGAIN, "if the atribute table already exists, restart may help");
			goto fail;
		}

		i++;
	}
	glite_jp_db_FreeStmt(&stmt);

	// feeds table
	if (glite_jp_db_PrepareStmt(jpctx, "INSERT INTO feeds (`state`, `locked`, `source`, `condition`) VALUES (?, ?, ?, ?)", &stmt) != 0) {
		glite_jpis_stack_error(ctx->jpctx, EIO, "can't create insert feeds statement");
		goto fail;
	}
	feeds = ctx->conf->feeds;
	i = 0;
	if (feeds) while (feeds[i]) {
		state = (feeds[i]->history ? GLITE_JP_IS_STATE_HIST : 0) |
		        (feeds[i]->continuous ? GLITE_JP_IS_STATE_CONT : 0);
		locked = 0;
		assert(glite_jpis_db_queries_serialize(ctx, &conds, &conds_len, feeds[i]->query) == 0);
		if (glite_jp_db_ExecPreparedStmt(jpctx, stmt, 4,
		  GLITE_LBU_DB_TYPE_INT, state,
		  GLITE_LBU_DB_TYPE_INT, locked,
		  GLITE_LBU_DB_TYPE_VARCHAR, feeds[i]->PS_URL,
		  GLITE_LBU_DB_TYPE_MEDIUMBLOB, conds, conds_len) == -1)
			goto fail_conds;
		free(conds);
		feeds[i]->uniqueid = glite_lbu_Lastid(stmt);

		i++;
	}
	glite_jp_db_FreeStmt(&stmt);

	return 0;

fail_conds:
	free(conds);
fail:
	glite_jp_db_FreeStmt(&stmt);
	if (!jpctx->error) glite_jpis_stack_error(ctx->jpctx, EIO, "error during initial filling of the database");
	return jpctx->error->code;
}


/* Drop the whole database. 
 *
 * \retval 0        OK
 * \retval non-zero JP error code
 */

int glite_jpis_dropDatabase(glite_jpis_context_t ctx) {
	char *attrid, sql[256];
	unsigned long len;
	int ret;
	glite_jp_context_t jpctx = ctx->jpctx;
	glite_lbu_Statement stmt_tabs = NULL;

	// search data tables and drop them
	if (glite_jp_db_PrepareStmt(jpctx, "SELECT attrid FROM attrs", &stmt_tabs) != 0) goto fail;
	if (glite_jp_db_ExecPreparedStmt(jpctx, stmt_tabs, 0) == -1) goto fail;
	while ((ret = glite_jp_db_FetchRow(jpctx, stmt_tabs, 1, &len, &attrid)) > 0) {
		snprintf(sql, sizeof(sql), SQLCMD_DROP_DATA_TABLE, attrid);
		llprintf(LOG_SQL, "dropping '%s' ==> '%s'\n", attrid, sql);
		if (glite_jp_db_ExecSQL(jpctx, sql, NULL) == -1) printf("warning: can't drop table '" TABLE_PREFIX_DATA "%s': %s (%s)\n", attrid, jpctx->error->desc, jpctx->error->source);
	}
	if (ret != 0) goto fail;
	glite_jp_db_FreeStmt(&stmt_tabs);

	// drop feeds and atributes
	if (glite_jp_db_ExecSQL(jpctx, "DELETE FROM attrs", NULL) == -1) goto fail;
	if (glite_jp_db_ExecSQL(jpctx, "DELETE FROM feeds", NULL) == -1) goto fail;
	if (glite_jp_db_ExecSQL(jpctx, "DELETE FROM jobs", NULL) == -1) goto fail;
	if (glite_jp_db_ExecSQL(jpctx, "DELETE FROM users", NULL) == -1) goto fail;
	if (glite_jp_db_ExecSQL(jpctx, "DELETE FROM acls", NULL) == -1) goto fail;

	return 0;

fail:
	glite_jp_db_FreeStmt(&stmt_tabs);
	return jpctx->error->code;
}


int glite_jpis_init_db(glite_jpis_context_t isctx) {
	int ret, caps;
	const char *cs;
	glite_jp_context_t jpctx;

	jpctx = isctx->jpctx;
	if (glite_lbu_InitDBContext(((glite_lbu_DBContext *)&jpctx->dbhandle)) != 0) goto fail_db;
	if ((cs = isctx->conf->cs) == NULL) cs = GLITE_JP_IS_DEFAULTCS;
	if (glite_lbu_DBConnect(jpctx->dbhandle, cs) != 0) goto fail_db;

	// try transaction for feeding
	if (isctx->conf->feeding) {
		caps = glite_lbu_DBQueryCaps(jpctx->dbhandle);
		if (caps != -1) {
			glite_lbu_DBSetCaps(jpctx->dbhandle, caps);
			llprintf(LOG_SQL, "transactions %s\n", (caps & GLITE_LBU_DB_CAP_TRANSACTIONS) ? "supported" : "not supported");
		}
	}

	// sql command: lock the feed (via uniqueid)
	if ((ret = glite_jp_db_PrepareStmt(jpctx, "UPDATE feeds SET locked=1 WHERE (locked = 0) AND (uniqueid = ?)", &isctx->lock_feed_stmt)) != 0) goto fail;

	// sql command: assign the feed (via uniqueid)
	if ((ret = glite_jp_db_PrepareStmt(jpctx, "UPDATE feeds SET feedid=?, expires=?, state=? WHERE (uniqueid=?)", &isctx->init_feed_stmt)) != 0) goto fail;

	// sql command: unlock the feed (via uniqueid)
	if ((ret = glite_jp_db_PrepareStmt(jpctx, "UPDATE feeds SET locked=0 WHERE (uniqueid=?)", &isctx->unlock_feed_stmt)) != 0) goto fail;

	// sql command: get info about the feed (via feedid)
	if ((ret = glite_jp_db_PrepareStmt(jpctx, "SELECT uniqueid, state, source FROM feeds WHERE (feedid=?)", &isctx->select_info_feed_stmt)) != 0) goto fail;

	// sql command: update state of the feed (via uniqueid)
	if ((ret = glite_jp_db_PrepareStmt(jpctx, "UPDATE feeds SET state=? WHERE (uniqueid=?)", &isctx->update_state_feed_stmt)) != 0) goto fail;

	// sql command: check for job with jobid
	if ((ret = glite_jp_db_PrepareStmt(jpctx, "SELECT jobid FROM jobs WHERE jobid=?", &isctx->select_jobid_stmt)) != 0) goto fail;

	// sql command: insert the job
	if ((ret = glite_jp_db_PrepareStmt(jpctx, "INSERT INTO jobs (jobid, dg_jobid, ownerid, ps) VALUES (?, ?, ?, ?)", &isctx->insert_job_stmt)) != 0) goto fail;

	// sql command: check the user
	if ((ret = glite_jp_db_PrepareStmt(jpctx, "SELECT userid FROM users WHERE userid=?", &isctx->select_user_stmt)) != 0) goto fail;

	// sql command: insert the user
	if ((ret = glite_jp_db_PrepareStmt(jpctx, "INSERT INTO users (userid, cert_subj) VALUES (?, ?)", &isctx->insert_user_stmt)) != 0) goto fail;

	return 0;

fail_db:
	ret = glite_jp_db_SetError(jpctx, __FUNCTION__);
fail:
	glite_jpis_free_db(isctx);
	return ret;
}


void glite_jpis_free_db(glite_jpis_context_t ctx) {
	glite_jp_db_FreeStmt(&ctx->lock_feed_stmt);
	glite_jp_db_FreeStmt(&ctx->init_feed_stmt);
	glite_jp_db_FreeStmt(&ctx->unlock_feed_stmt);
	glite_jp_db_FreeStmt(&ctx->select_info_feed_stmt);
	glite_jp_db_FreeStmt(&ctx->update_state_feed_stmt);
	glite_jp_db_FreeStmt(&ctx->select_jobid_stmt);
	glite_jp_db_FreeStmt(&ctx->select_user_stmt);
	glite_jp_db_FreeStmt(&ctx->insert_job_stmt);
	glite_jp_db_FreeStmt(&ctx->insert_user_stmt);
	glite_lbu_DBClose(ctx->jpctx->dbhandle);
	glite_lbu_FreeDBContext(ctx->jpctx->dbhandle);
	ctx->jpctx->dbhandle = NULL;
}


/* Find first unitialized feed, lock it and return URL of corresponding PS 
 *
 * Return value:
 *      0      - OK
 *      ENOENT - no more feeds to initialize
 *      ENOLCK - error during locking */

int glite_jpis_lockSearchFeed(glite_jpis_context_t ctx, int initialized, long int *uniqueid, char **PS_URL, int *status, char **feedid)
{
	int ret;
	static int uninit_msg = 1;
	char *sql, *res[4], *t, *ps;
	glite_lbu_Statement stmt;

	if (feedid) *feedid = NULL;
	do {
		glite_lbu_TimeToDB(time(NULL), &t);
		if (initialized) {
			trio_asprintf(&sql, "SELECT uniqueid, source, state, feedid FROM feeds WHERE (locked=0) AND (feedid IS NOT NULL) AND (expires <= %s)", t);
		} else
			trio_asprintf(&sql, "SELECT uniqueid, source, state, feedid FROM feeds WHERE (locked=0) AND (feedid IS NULL) AND ((state < " GLITE_JP_IS_STATE_ERROR_STR ") OR (expires <= %s))", t);
		free(t);
		//llprintf(LOG_SQL, "sql=%s\n", sql);
		ret = glite_jp_db_ExecSQL(ctx->jpctx, sql, &stmt);
		free(sql);
		switch (ret) {
		case -1:
			glite_jpis_stack_error(ctx->jpctx, ENOLCK, "error selecting unlocked feed");
			uninit_msg = 1;
			glite_jp_db_FreeStmt(&stmt);
			return ENOLCK;
		case 0:
			if (uninit_msg) {
				lprintf("no more %s feeds for now\n", initialized ? "not-refreshed" : "uninitialized");
				uninit_msg = 0;
			}
			glite_jp_db_FreeStmt(&stmt);
			return ENOENT;
		default: break;
		}
		uninit_msg = 1;
		if (glite_jp_db_FetchRow(ctx->jpctx, stmt, sizeof(res)/sizeof(res[0]), NULL, res) <= 0) {
			glite_jpis_stack_error(ctx->jpctx, ENOLCK, "error fetching unlocked feed");
			glite_jp_db_FreeStmt(&stmt);
			return ENOLCK;
		}
		glite_jp_db_FreeStmt(&stmt);
		lprintf("selected feed, uniqueid=%s\n", res[0]);
		*uniqueid = atol(res[0]); free(res[0]);
		ps = res[1];
		if (status) *status = atoi(res[2]); free(res[2]);
		if (feedid) {
			free(*feedid);
			*feedid = res[3];
		} else free(res[3]);

		ret = glite_jp_db_ExecPreparedStmt(ctx->jpctx, ctx->lock_feed_stmt, 1, GLITE_LBU_DB_TYPE_INT, *uniqueid);
		lprintf("locked %d feeds (uniqueid=%ld)\n", ret, *uniqueid);
	} while (ret != 1);

	if (PS_URL) *PS_URL = ps;
	else free(ps);

	return 0;
}


/* Store feed ID and expiration time returned by PS for locked feed. */

int glite_jpis_initFeed(glite_jpis_context_t ctx, long int uniqueid, const char *feedId, time_t feedExpires, int status)
{
	int ret;
	time_t tnow, expires;

	tnow = time(NULL);
	expires = tnow + (feedExpires - tnow) / 2;

	ret = glite_jp_db_ExecPreparedStmt(ctx->jpctx, ctx->init_feed_stmt, 4,
		GLITE_LBU_DB_TYPE_CHAR, feedId,
		GLITE_LBU_DB_TYPE_DATETIME, expires,
		GLITE_LBU_DB_TYPE_INT, status,
		GLITE_LBU_DB_TYPE_INT, uniqueid);
	lprintf("initializing feed, uniqueid=%ld, result=%d\n", uniqueid, ret);

	return ret == 1 ? 0 : ENOLCK;
}


/* Unlock given feed */

int glite_jpis_unlockFeed(glite_jpis_context_t ctx, long int uniqueid) {
	int ret;

	ret = glite_jp_db_ExecPreparedStmt(ctx->jpctx, ctx->unlock_feed_stmt, 1, GLITE_LBU_DB_TYPE_INT, uniqueid);
	lprintf("unlocking feed, uniqueid=%ld, result=%d\n", uniqueid, ret);

	return ret == 1 ? 0 : ENOLCK;
}


/* Saves TTL (when to reconnect if error occured) for given feed */

int glite_jpis_tryReconnectFeed(glite_jpis_context_t ctx, long int uniqueid, time_t reconn_time, int state) {
	int ret;
	char *sql, *t;

	glite_lbu_TimeToDB(reconn_time, &t);
	lprintf("reconnect, un=%ld, %s\n", uniqueid, t);
	trio_asprintf(&sql, "UPDATE feeds SET state=%d, expires=%s WHERE (uniqueid=%ld)", state, t, uniqueid);
	free(t);
	if ((ret = glite_jp_db_ExecSQL(ctx->jpctx, sql, NULL)) != 1)
		glite_jpis_stack_error(ctx->jpctx, EIO, "can't update feed no. %ld in DB", uniqueid);
	free(sql);
	return ret == -1 ? ctx->jpctx->error->code : 0;
}


// TODO: could be merged with initFeed
int glite_jpis_destroyTryReconnectFeed(glite_jpis_context_t ctx, long int uniqueid, time_t reconn_time) {
	int ret;
	char *sql, *t;

	glite_lbu_TimeToDB(reconn_time, &t);
	lprintf("destroy not refreshed feed, un=%ld, %s\n", uniqueid, t);
	trio_asprintf(&sql, "UPDATE feeds SET feedid=NULL, state=0, expires=%s WHERE (uniqueid=%ld)", t, uniqueid);
	free(t);
	if ((ret = glite_jp_db_ExecSQL(ctx->jpctx, sql, NULL)) != 1)
		glite_jpis_stack_error(ctx->jpctx, EIO, "can't destroy non-refreshable feed no. %ld in DB", uniqueid);
	free(sql);
	return ret == -1 ? ctx->jpctx->error->code : 0;
}


int glite_jpis_insertAttrVal(glite_jpis_context_t ctx, const char *jobid, glite_jp_attrval_t *av) {
	char *sql, *table, *value, *full_value, *md5_jobid;
	long int origin;

	table = glite_jpis_attr_name2id(av->name);
	value = glite_jp_attrval_to_db_index(ctx->jpctx, av, INDEX_LENGTH);
	full_value = glite_jp_attrval_to_db_full(ctx->jpctx, av);
	md5_jobid = str2md5(jobid);
	origin = av->origin;
	trio_asprintf(&sql, SQLCMD_INSERT_ATTRVAL, table, md5_jobid, value, full_value, origin);
	free(md5_jobid);
	free(table);
	free(value);
	free(full_value);
	llprintf(LOG_SQL, "(%s) sql=%s\n", av->name, sql);
//	if (ctx->conf->feeding) printf("FEED: %s\n", sql);
//	else
	if (glite_jp_db_ExecSQL(ctx->jpctx, sql, NULL) != 1) {
		free(sql);
		return ctx->jpctx->error->code;
	}
	free(sql);

	return 0;
}


int glite_jpis_lazyInsertJob(glite_jpis_context_t ctx, const char *ps, const char *jobid, const char *owner) {
	int ret;
	char *md5_jobid = NULL, *md5_cert = NULL;

	lprintf("\n");

	if (!jobid || !owner) {
		glite_jpis_stack_error(ctx->jpctx, EINVAL, "jobid and owner is mandatory (jobid=%s, owner=%s)!\n", jobid, owner);
		goto fail;
	}
	md5_jobid = str2md5(jobid);
	md5_cert = str2md5(owner);
	switch (ret = glite_jp_db_ExecPreparedStmt(ctx->jpctx, ctx->select_jobid_stmt, 1, GLITE_LBU_DB_TYPE_CHAR, md5_jobid)) {
	case 1: lprintf("jobid '%s' found\n", jobid); goto ok0;
	case 0:
		lprintf("inserting jobid %s (%s)\n", jobid, md5_jobid);
		if (glite_jp_db_ExecPreparedStmt(ctx->jpctx, ctx->insert_job_stmt, 4,
			GLITE_LBU_DB_TYPE_CHAR, md5_jobid,
			GLITE_LBU_DB_TYPE_VARCHAR, jobid,
			GLITE_LBU_DB_TYPE_CHAR, md5_cert,
			GLITE_LBU_DB_TYPE_CHAR, ps) != 1) goto fail;
		break;
	default: assert(ret != 1); break;
	}
ok0:

	switch (ret = glite_jp_db_ExecPreparedStmt(ctx->jpctx, ctx->select_user_stmt, 1, GLITE_LBU_DB_TYPE_CHAR, md5_cert)) {
	case 1: lprintf("owner '%s' found\n", owner); goto ok;
	case 0:
		lprintf("inserting user %s (%s)\n", owner, md5_cert);
		if (glite_jp_db_ExecPreparedStmt(ctx->jpctx, ctx->insert_user_stmt, 2,
			GLITE_LBU_DB_TYPE_CHAR, md5_cert,
			GLITE_LBU_DB_TYPE_VARCHAR, owner) != 1) goto fail;
		break;
	default: assert(ret != 1); break;
	}

ok:
	free(md5_jobid);
	free(md5_cert);
	return 0;
fail:
	free(md5_jobid);
	free(md5_cert);
	return ctx->jpctx->error->code;
}


#define FEEDING_SEPARATORS ";"
#define FEEDING_JOBID_BKSERVER "localhost-test"
#define FEEDING_JOBID_PORT 0
#define FEEDING_PRIMARY_STORAGE "localhost:8901"
#define FEEDING_DEFAULT_OWNER "God"
int glite_jpis_feeding(glite_jpis_context_t ctx, const char *fname, const char *dn) {
	FILE *f;
	char line[1024], *token, *lasts, *jobid = NULL;
	int nattrs, lno, i, iname, c;
	glite_jp_attrval_t *avs;
	glite_jobid_t j;
	const char *owner = dn ? dn : FEEDING_DEFAULT_OWNER;

	if ((f = fopen(fname, "rt")) == NULL) {
		glite_jpis_stack_error(ctx->jpctx, errno, "can't open csv dump file");
		return 1;
	}

	for (nattrs = 0; ctx->conf->attrs[nattrs]; nattrs++);
	avs = malloc(nattrs * sizeof avs[0]);

	lno = 0;
	while(fgets(line, sizeof line, f) != NULL) {
		if ((lno % 100) == 0) {
			if (lno) glite_jp_db_Commit(ctx->jpctx);
			glite_jp_db_Transaction(ctx->jpctx);
		}
		lno++;
		if (line[0]) {
			c = strlen(line) - 1;
			if (line[c] != '\r' && line[c] != '\n' && !feof(f)) {
				glite_jpis_stack_error(ctx->jpctx, E2BIG, "line too large at %d (max. %d)", lno, sizeof line);
				goto err;
			}
			while (c >= 0 && (line[c] == '\r' || line[c] == '\n')) c--;
			line[c + 1] = 0;
		}
//		printf("'%s'\n", line);

		memset(avs, 0, nattrs * sizeof avs[0]);
		i = 0;
		iname = 0;
		token = strtok_r(line, FEEDING_SEPARATORS, &lasts);
		while (token && iname < nattrs) {
//			printf("\t'%s'\n", token);
			do {
				avs[i].name = ctx->conf->attrs[iname];
				iname++;
			} while (strcasecmp(avs[i].name, GLITE_JP_ATTR_JOBID) == 0 || strcasecmp(avs[i].name, GLITE_JP_ATTR_OWNER) == 0);
			avs[i].value = token;
			avs[i].timestamp = time(NULL);
//			printf(stderr, "\t %d: %s = '%s'\n", i, avs[i].name, avs[i].value);
			i++;

			token = strtok_r(NULL, FEEDING_SEPARATORS, &lasts);
		}

		if (glite_jobid_create(FEEDING_JOBID_BKSERVER, FEEDING_JOBID_PORT, &j) != 0) {
			glite_jpis_stack_error(ctx->jpctx, errno, "can't create jobid");
			goto err;
		}
		if ((jobid = glite_jobid_unparse(j)) == NULL) {
			glite_jobid_free(j);
			glite_jpis_stack_error(ctx->jpctx, ENOMEM, "can't unparse jobid");
			goto err;
		}
		glite_jobid_free(j);
		if (glite_jpis_lazyInsertJob(ctx, FEEDING_PRIMARY_STORAGE, jobid, owner)) goto err;
		for (i = 0; i < nattrs && avs[i].name; i++) {
			if (glite_jpis_insertAttrVal(ctx, jobid, &avs[i])) goto err;
		}
		free(jobid); jobid = NULL;
	}
	glite_jp_db_Commit(ctx->jpctx);

	fclose(f);
	free(avs);
	return 0;
err:
	fclose(f);
	free(avs);
	free(jobid);
	glite_jp_db_Rollback(ctx->jpctx);
	return 1;
}
