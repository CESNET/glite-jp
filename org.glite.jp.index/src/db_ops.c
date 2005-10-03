#ident "$Header$"

#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include <glite/jp/types.h>
#include <glite/jp/context.h>
#include <glite/jp/db.h>
#include <glite/jp/attr.h>
#include <glite/jp/strmd5.h>

#include "conf.h"
#include "db_ops.h"


#define TABLE_PREFIX_DATA "attr_"
#define SQLCMD_DROP_DATA_TABLE "DROP TABLE " TABLE_PREFIX_DATA "%s"
#define SQLCMD_CREATE_DATA_TABLE "CREATE TABLE " TABLE_PREFIX_DATA "%s (\n\
        jobid          CHAR(32)    BINARY NOT NULL,\n\
        value          %s          BINARY NOT NULL,\n\
        full_value     %s          NOT NULL,\n\
\n\
        INDEX (jobid),\n\
        INDEX (value)\n\
);"
#define SQLCMD_INSERT_ATTRVAL "INSERT INTO " TABLE_PREFIX_DATA "%s (jobid, value, full_value) VALUES (\n\
	'%s',\n\
	'%s',\n\
	'%s'\n\
)"
#define INDEX_LENGTH 255

//#define lprintf
#define lprintf printf

#define WORD_SWAP(X) ((((X) >> 8) & 0xFF) | (((X) & 0xFF) << 8))
#define LONG_SWAP(X) (WORD_SWAP(((X) >> 16) & 0xFFFF) | ((WORD_SWAP(X) & 0xFFFF) << 16))
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define LONG_LE(X) (X)
#else
#define LONG_LE(X) LONG_SWAP(X)
#endif

#define COND_MAGIC 0x444E4F43

static int is_indexed(glite_jp_is_conf *conf, const char *attr) {
	size_t i;

	i = 0;
	while (conf->indexed_attrs[i]) {
		if (strcasecmp(attr, conf->indexed_attrs[i]) == 0) return 1;
		i++;
	}
	return 0;
}


static size_t db_arg2_length(glite_jp_query_rec_t *query) {
	size_t len;

	assert(query->op > GLITE_JP_QUERYOP_UNDEF && query->op <= GLITE_JP_QUERYOP__LAST);
	len = 0;
	switch (query->op) {
		case GLITE_JP_QUERYOP_WITHIN:
			len = query->binary ? query->size2 : strlen(query->value2) + 1;
		case GLITE_JP_QUERYOP_UNDEF:
		case GLITE_JP_QUERYOP_EQUAL:
		case GLITE_JP_QUERYOP_UNEQUAL:
		case GLITE_JP_QUERYOP_LESS:
		case GLITE_JP_QUERYOP_GREATER:
		case GLITE_JP_QUERYOP_EXISTS:
		case GLITE_JP_QUERYOP__LAST:
			len = 0;
	}

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


static int glite_jpis_db_queries_serialize(void **blob, size_t *len, glite_jp_query_rec_t **queries) {
	size_t maxlen;
	glite_jp_query_rec_t *query;
	int ret;
	size_t datalen;

	if ((ret = array_init(blob, len, &maxlen, 1024)) != 0) return ret;
	query = *queries;
	while(query && query->attr) {
		if ((ret = array_add_long(blob, len, &maxlen, COND_MAGIC)) != 0) goto fail;
		datalen = strlen(query->attr) + 1;
		if ((ret = array_add_long(blob, len, &maxlen, datalen)) != 0) goto fail;
		if ((ret = array_add(blob, len, &maxlen, query->attr, datalen)) != 0) goto fail;
		if ((ret = array_add_long(blob, len, &maxlen, query->op)) != 0) goto fail;
		if ((ret = array_add_long(blob, len, &maxlen, query->binary ? 1 : 0)) != 0) goto fail;

		datalen = query->binary ? query->size : strlen(query->value) + 1;
		if ((ret = array_add_long(blob, len, &maxlen, datalen)) != 0) goto fail;
		if (datalen)
			if ((ret = array_add(blob, len, &maxlen, query->value, datalen)) != 0) goto fail;
		
		datalen = db_arg2_length(query);
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


static int glite_jpis_db_queries_deserialize(glite_jp_query_rec_t ***queries, void *blob, size_t blob_size) {
	size_t maxlen, len, datalen;
	void *blob_ptr, *blob_end;
	int ret;
	uint32_t l;
	glite_jp_query_rec_t *query;
	int i;

	if ((ret = array_init((void **)queries, &len, &maxlen, 512)) != 0) return ret;
	blob_ptr = blob;
	blob_end = (char *)blob + blob_size;
	while (blob_end > blob_ptr) {
		ret = ENOMEM;
		if ((query = calloc(sizeof(*query), 1)) == NULL) goto fail;
		l = array_get_long(&blob_ptr);
		if (l != COND_MAGIC) {
			lprintf("blob=%p, blob_ptr=%p, 0x%08" PRIX32 "\n", blob, blob_ptr, l);
			ret = EINVAL;
			goto fail_query;
		}

		datalen = array_get_long(&blob_ptr);
		if (datalen) {
			if ((query->attr = malloc(datalen)) == NULL) goto fail_query;
			memcpy(query->attr, array_get(&blob_ptr, datalen), datalen);
		} else query->attr = NULL;

		query->op = array_get_long(&blob_ptr);
		query->binary = array_get_long(&blob_ptr);

		datalen = array_get_long(&blob_ptr);
		if (datalen) {
			if ((query->value = malloc(datalen)) == NULL) goto fail_query;
			memcpy(query->value, array_get(&blob_ptr, datalen), datalen);
		} else query->value = NULL;
		query->size = datalen;

		datalen = array_get_long(&blob_ptr);
		if (datalen) {
			if ((query->value2 = malloc(datalen)) == NULL) goto fail_query;
			memcpy(query->value2, array_get(&blob_ptr, datalen), datalen);
		} else query->value2 = NULL;
		query->size2 = datalen;

		if ((ret = array_add((void **)queries, &len, &maxlen, &query, sizeof(query))) != 0) goto fail_query;
	}
	assert(blob_end == blob_ptr);

	query = NULL;
	if ((ret = array_add((void **)queries, &len, &maxlen, &query, sizeof(query))) != 0) goto fail;

	return 0;

fail_query:
	free(query);
fail:
	i = 0;
	query = (*queries)[i];
	while (query && query->attr) {
		free(query->attr);
		free(query->value);
		free(query->value2);
		free(query);
		i++;
		query = (*queries)[i];
	}
	free(*queries);
	return ret;
}


/**
 * Convert attribute name to attribute id.
 */
char *glite_jpis_attr_name2id(const char *name) {
	return str2md5(name);
}


/* Init the database. 
 *
 * \retval 0        OK
 * \retval non-zero JP error code
 */

int glite_jpis_initDatabase(glite_jp_context_t ctx, glite_jp_is_conf *conf) {
	glite_jp_db_stmt_t stmt;
	char **attrs, *tmp;
	const char *type_index, *type_full;
	size_t i;
	void *param;
	unsigned long attrid_len, name_len, type_len, source_len, dbconds_len;
	char attrid[33], name[256], type[33], source[256], dbconds[1024];
	int indexed, state, locked;
	size_t conds_len;
	char sql[512];
	glite_jp_is_feed **feeds;
	void *conds;

	glite_jp_db_create_params(&param, 4,
		GLITE_JP_DB_TYPE_VARCHAR, attrid, &attrid_len,
		GLITE_JP_DB_TYPE_VARCHAR, name, &name_len,
		GLITE_JP_DB_TYPE_INT, &indexed,
		GLITE_JP_DB_TYPE_VARCHAR, type, &type_len);
	if (glite_jp_db_prepare(ctx, "INSERT INTO attrs (attrid, name, indexed, type) VALUES (?, ?, ?, ?)", &stmt, param, NULL) != 0) goto fail;

	memset(attrid, 0, sizeof(attrid));

	// attrs table and attrid_* tables
	attrs = conf->attrs;
	i = 0;
	while (attrs[i]) {
		type_full = glite_jp_attrval_db_type_full(ctx, attrs[i]);
		type_index = glite_jp_attrval_db_type_index(ctx, attrs[i], INDEX_LENGTH);

		// attrid column
		tmp = glite_jpis_attr_name2id(attrs[i]);
		strncpy(attrid, tmp, sizeof(attrid) - 1);
		free(tmp);
		attrid_len = strlen(attrid);
		// attr name column
		strncpy(name, attrs[i], sizeof(name) - 1);
		name_len = strlen(name);
		// indexed column
		indexed = is_indexed(conf, name);
		// type column
		strncpy(type, type_full, sizeof(type) - 1);
		type_len = strlen(type);
		// insert
		if (glite_jp_db_execute(stmt) == -1) goto fail_stmt;

		snprintf(sql, sizeof(sql), SQLCMD_CREATE_DATA_TABLE, attrid, type_index, type_full);
		lprintf("creating table: '%s'\n", sql);
		if ((glite_jp_db_execstmt(ctx, sql, NULL)) == -1) goto fail_stmt;

		i++;
	}
	glite_jp_db_freestmt(&stmt);

	// feeds table
	glite_jp_db_create_params(&param, 4,
		GLITE_JP_DB_TYPE_INT, &state,
		GLITE_JP_DB_TYPE_INT, &locked,
		GLITE_JP_DB_TYPE_VARCHAR, source, &source_len,
		GLITE_JP_DB_TYPE_MEDIUMBLOB, dbconds, &dbconds_len);
	if (glite_jp_db_prepare(ctx, "INSERT INTO feeds (state, locked, source, condition) VALUES (?, ?, ?, ?)", &stmt, param, NULL) != 0) goto fail;
	feeds = conf->feeds;
	i = 0;
	memset(source, 0, sizeof(source));
	while (feeds[i]) {
		state = (feeds[i]->history ? GLITE_JP_IS_STATE_HIST : 0) |
		        (feeds[i]->continuous ? GLITE_JP_IS_STATE_CONT : 0);
		locked = 0;
		strncpy(source, feeds[i]->PS_URL, sizeof(source) - 1);
		source_len = strlen(source);
		assert(glite_jpis_db_queries_serialize(&conds, &conds_len, feeds[i]->query) == 0);
		assert(conds_len <= sizeof(dbconds));
		dbconds_len = conds_len;
		memcpy(dbconds, conds, conds_len);
		free(conds);
		if (glite_jp_db_execute(stmt) == -1) goto fail_conds;

		i++;
	}
	glite_jp_db_freestmt(&stmt);

	return 0;

fail_conds:
	free(conds);
fail_stmt:
	glite_jp_db_freestmt(&stmt);
fail:
	return ctx->error->code;
}


/* Drop the whole database. 
 *
 * \retval 0        OK
 * \retval non-zero JP error code
 */

int glite_jpis_dropDatabase(glite_jp_context_t ctx) {
	glite_jp_db_stmt_t stmt_tabs;
	void *res;
	char attrid[33], sql[256];
	unsigned long len;
	int ret;

	// search data tables and drop them
	glite_jp_db_create_results(&res, 1, GLITE_JP_DB_TYPE_CHAR, NULL, attrid, sizeof(attrid), &len);
	if (glite_jp_db_prepare(ctx, "SELECT attrid FROM attrs", &stmt_tabs, NULL, res) != 0) goto fail;
	if (glite_jp_db_execute(stmt_tabs) == -1) goto fail_tabs;
	while ((ret = glite_jp_db_fetch(stmt_tabs)) == 0) {
		snprintf(sql, sizeof(sql), SQLCMD_DROP_DATA_TABLE, attrid);
		lprintf("dropping '%s' ==> '%s'\n", attrid, sql);
		if (glite_jp_db_execstmt(ctx, sql, NULL) == -1) printf("warning: can't drop table '" TABLE_PREFIX_DATA "%s': %s (%s)\n", attrid, ctx->error->desc, ctx->error->source);
	}
	if (ret != ENODATA) goto fail_tabs;
	glite_jp_db_freestmt(&stmt_tabs);

	// drop feeds and atributes
	if (glite_jp_db_execstmt(ctx, "DELETE FROM attrs", NULL) == -1) goto fail;
	if (glite_jp_db_execstmt(ctx, "DELETE FROM feeds", NULL) == -1) goto fail;

	return 0;

fail_tabs:
	glite_jp_db_freestmt(&stmt_tabs);
fail:
	return ctx->error->code;
}


int glite_jpis_init_context(glite_jpis_context_t *isctx, glite_jp_context_t jpctx) {
	int ret;
	void *myparam;
	void *myres;
	const char *cs;

	*isctx = calloc(sizeof(**isctx), 1);
	
	(*isctx)->jpctx = jpctx;
	if ((cs = getenv("GLITE_JPIS_DB")) == NULL) cs = GLITE_JP_IS_DEFAULTCS;
	if ((ret = glite_jp_db_connect(jpctx, cs)) != 0) goto fail;

	// sql command: select an uninitialized unlocked feed
	glite_jp_db_create_results(&myres, 2,
		GLITE_JP_DB_TYPE_INT, NULL, &((*isctx)->param_uniqueid),
		GLITE_JP_DB_TYPE_VARCHAR, NULL, (*isctx)->param_ps, sizeof((*isctx)->param_ps), &(*isctx)->param_ps_len);
	if ((ret = glite_jp_db_prepare(jpctx, "SELECT uniqueid, source FROM feeds WHERE (locked=0) AND (feedid IS NULL)", &(*isctx)->select_unlocked_feed_stmt, NULL, myres)) != 0) goto fail_connect;

	// sql command: lock the feed (via uniqueid)
	glite_jp_db_create_params(&myparam, 1, GLITE_JP_DB_TYPE_INT, &(*isctx)->param_uniqueid);
	if ((ret = glite_jp_db_prepare(jpctx, "UPDATE feeds SET locked=1 WHERE (locked = 0) AND (uniqueid = ?)", &(*isctx)->lock_feed_stmt, myparam, NULL)) != 0) goto fail_cmd;

	// sql command: assign the feed (via uniqueid)
	glite_jp_db_create_params(&myparam, 3,
		GLITE_JP_DB_TYPE_VARCHAR, (*isctx)->param_feedid, &(*isctx)->param_feedid_len,
		GLITE_JP_DB_TYPE_DATETIME, &(*isctx)->param_expires,
		GLITE_JP_DB_TYPE_INT, &(*isctx)->param_uniqueid);
	if ((ret = glite_jp_db_prepare(jpctx, "UPDATE feeds SET feedid=?, expires=? WHERE (uniqueid=?)", &(*isctx)->init_feed_stmt, myparam, NULL)) != 0) goto fail_cmd2;

	// sql command: unlock the feed (via uniqueid)
	glite_jp_db_create_params(&myparam, 1, GLITE_JP_DB_TYPE_INT, &(*isctx)->param_uniqueid);
	if ((ret = glite_jp_db_prepare(jpctx, "UPDATE feeds SET locked=0 WHERE (uniqueid=?)", &(*isctx)->unlock_feed_stmt, myparam, NULL)) != 0) goto fail_cmd3;

	// sql command: get info about the feed (via feedid)
	glite_jp_db_create_params(&myparam, 1, GLITE_JP_DB_TYPE_CHAR, (*isctx)->param_feedid, &(*isctx)->param_feedid_len);
	glite_jp_db_create_results(&myres, 2,
		GLITE_JP_DB_TYPE_INT, NULL, &(*isctx)->param_uniqueid,
		GLITE_JP_DB_TYPE_INT, NULL, &(*isctx)->param_state);
	if ((ret = glite_jp_db_prepare(jpctx, "SELECT uniqueid, state FROM feeds WHERE (feedid=?)", &(*isctx)->select_info_feed_stmt, myparam, myres)) != 0) goto fail_cmd4;

	// sql command: update state of the feed (via uniqueid)
	glite_jp_db_create_params(&myparam, 2, 
		GLITE_JP_DB_TYPE_INT, &(*isctx)->param_state,
		GLITE_JP_DB_TYPE_INT, &(*isctx)->param_uniqueid);
	if ((ret = glite_jp_db_prepare(jpctx, "UPDATE feeds SET state=? WHERE (uniqueid=?)", &(*isctx)->update_state_feed_stmt, myparam, NULL)) != 0) goto fail_cmd5;

	return 0;

	glite_jp_db_freestmt(&(*isctx)->update_state_feed_stmt);
fail_cmd5:
	glite_jp_db_freestmt(&(*isctx)->select_info_feed_stmt);
fail_cmd4:
	glite_jp_db_freestmt(&(*isctx)->unlock_feed_stmt);
fail_cmd3:
	glite_jp_db_freestmt(&(*isctx)->init_feed_stmt);
fail_cmd2:
	glite_jp_db_freestmt(&(*isctx)->lock_feed_stmt);
fail_cmd:
	glite_jp_db_freestmt(&(*isctx)->select_unlocked_feed_stmt);
fail_connect:
	glite_jp_db_close((*isctx)->jpctx);
fail:
	free(*isctx);
	return ret;
}


void glite_jpis_free_context(glite_jpis_context_t ctx) {
	glite_jp_db_freestmt(&ctx->select_unlocked_feed_stmt);
	glite_jp_db_freestmt(&ctx->lock_feed_stmt);
	glite_jp_db_freestmt(&ctx->init_feed_stmt);
	glite_jp_db_freestmt(&ctx->unlock_feed_stmt);
	glite_jp_db_freestmt(&ctx->select_info_feed_stmt);
	glite_jp_db_freestmt(&ctx->update_state_feed_stmt);
	glite_jp_db_close(ctx->jpctx);
	free(ctx);
}


/* Find first unitialized feed, lock it and return URL of corresponding PS 
 *
 * Return value:
 *      0      - OK
 *      ENOENT - no more feeds to initialize
 *      ENOLCK - error during locking */

int glite_jpis_lockUninitializedFeed(glite_jpis_context_t ctx, long int *uniqueid, char **PS_URL)
{
	int ret;

	do {
		switch (glite_jp_db_execute(ctx->select_unlocked_feed_stmt)) {
		case -1: lprintf("error selecting unlocked feed\n"); return ENOLCK;
		case 0: lprintf("no more uninit. feeds unlocked\n"); return ENOENT;
		default: break;
		}
		if (glite_jp_db_fetch(ctx->select_unlocked_feed_stmt) != 0) return ENOLCK;
		lprintf("selected uninit. feed %lu\n", ctx->param_uniqueid);

		ret = glite_jp_db_execute(ctx->lock_feed_stmt);
		lprintf("locked %d feeds (uniqueid=%lu)\n", ret, ctx->param_uniqueid);
	} while (ret != 1);

	*uniqueid = ctx->param_uniqueid;
	if (PS_URL) *PS_URL = strdup(ctx->param_ps);

	return 0;
}


/* Store feed ID and expiration time returned by PS for locked feed. */

int glite_jpis_initFeed(glite_jpis_context_t ctx, long int uniqueid, char *feedId, time_t feedExpires)
{
	int ret;

	memset(ctx->param_feedid, 0, sizeof(ctx->param_feedid));
	strncpy(ctx->param_feedid, feedId, sizeof(ctx->param_feedid) - 1);
	ctx->param_feedid_len = strlen(ctx->param_feedid) + 1;
	glite_jp_db_set_time(ctx->param_expires, feedExpires);
	ctx->param_uniqueid = uniqueid;

	ret = glite_jp_db_execute(ctx->init_feed_stmt);
	lprintf("initializing feed, uniqueid=%li, result=%d\n", uniqueid, ret);

	return ret == 1 ? 0 : ENOLCK;
}


/* Unlock given feed */

int glite_jpis_unlockFeed(glite_jpis_context_t ctx, long int uniqueid) {
	int ret;

	ctx->param_uniqueid = uniqueid;
	ret = glite_jp_db_execute(ctx->unlock_feed_stmt);
	lprintf("unlocking feed, uniqueid=%li, result=%d\n", uniqueid, ret);

	return ret == 1 ? 0 : ENOLCK;
}


int glite_jpis_insertAttrVal(glite_jpis_context_t ctx, const char *jobid, glite_jp_attrval_t *av) {
	char *sql, *table, *value, *full_value;

	table = glite_jpis_attr_name2id(av->name);
	value = glite_jp_attrval_to_db_index(ctx->jpctx, av, INDEX_LENGTH);
	full_value = glite_jp_attrval_to_db_full(ctx->jpctx, av);
	asprintf(&sql, SQLCMD_INSERT_ATTRVAL, table, jobid, value, full_value);
	free(table);
	free(value);
	free(full_value);
	lprintf("%s: sql=%s\n", __FUNCTION__, sql);
	if (glite_jp_db_execstmt(ctx->jpctx, sql, NULL) != 1) {
		free(sql);
		return ctx->jpctx->error->code;
	}
	free(sql);

	return 0;
}
