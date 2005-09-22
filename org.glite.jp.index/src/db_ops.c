#ident "$Header$"

#include <time.h>
#include <errno.h>

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

#define lprintf
//#define lprintf printf


static is_indexed(glite_jp_is_conf *conf, const char *attr) {
	size_t i;

	i = 0;
	while (conf->indexed_attrs[i]) {
		if (strcasecmp(attr, conf->indexed_attrs[i]) == 0) return 1;
		i++;
	}
	return 0;
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
	MYSQL_BIND param[4];
	unsigned long attrid_len, name_len, type_len, source_len;
	char attrid[33], name[256], type[33], source[256];
	int indexed, state, locked;
	char sql[512];
	glite_jp_is_feed **feeds;

	glite_jp_db_assign_param(&param[0], MYSQL_TYPE_VAR_STRING, attrid, &attrid_len);
	glite_jp_db_assign_param(&param[1], MYSQL_TYPE_VAR_STRING, name, &name_len);
	glite_jp_db_assign_param(&param[2], MYSQL_TYPE_LONG, &indexed);
	glite_jp_db_assign_param(&param[3], MYSQL_TYPE_VAR_STRING, type, &type_len);
	if (glite_jp_db_prepare(ctx, "INSERT INTO attrs (attrid, name, indexed, type) VALUES (?, ?, ?, ?)", &stmt, param, NULL) != 0) goto fail;

	memset(attrid, 0, sizeof(attrid));

	// attrs table and attrid_* tables
	attrs = conf->attrs;
	i = 0;
	while (attrs[i]) {
		type_full = glite_jp_attrval_db_type_full(ctx, attrs[i]);
		type_index = glite_jp_attrval_db_type_index(ctx, attrs[i], 255);

		// attrid column
		tmp = str2md5(attrs[i]);
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
	glite_jp_db_assign_param(&param[0], MYSQL_TYPE_LONG, &state);
	glite_jp_db_assign_param(&param[1], MYSQL_TYPE_LONG, &locked);
	glite_jp_db_assign_param(&param[2], MYSQL_TYPE_VAR_STRING, source, &source_len);
	if (glite_jp_db_prepare(ctx, "INSERT INTO feeds (state, locked, source) VALUES (?, ?, ?)", &stmt, param, NULL) != 0) goto fail;
	feeds = conf->feeds;
	i = 0;
	memset(source, 0, sizeof(source));
	while (feeds[i]) {
		state = (feeds[i]->history ? GLITE_JP_IS_STATE_HIST : 0) |
		        (feeds[i]->continuous ? GLITE_JP_IS_STATE_CONT : 0);
		locked = 0;
		strncpy(source, feeds[i]->PS_URL, sizeof(source) - 1);
		source_len = strlen(source);
		if (glite_jp_db_execute(stmt) == -1) goto fail_stmt;

		i++;
	}
	glite_jp_db_freestmt(&stmt);

	return 0;

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
	MYSQL_BIND inp, res;
	char attrid[33], sql[256];
	unsigned long len;
	int ret;

	// search data tables and drop them
	glite_jp_db_assign_result(&res, MYSQL_TYPE_STRING, NULL, attrid, sizeof(attrid), &len);
	if (glite_jp_db_prepare(ctx, "SELECT attrid FROM attrs", &stmt_tabs, NULL, &res) != 0) goto fail;
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


/* Find first unitialized feed, lock it and return URL of corresponding PS 
 *
 * Return value:
 *      0      - OK
 *      ENOENT - no more feeds to initialize
 *      ENOLCK - error during locking */

int glite_jpis_lockUninitializedFeed(glite_jp_context_t ctx, char **PS_URL)
{
	return ENOENT;
}


/* Store feed ID and expiration time returned by PS for locked feed. */

void glite_jpis_feedInit(glite_jp_context_t ctx, char *PS_URL, char *feedId, time_t feedExpires)
{
}

/* Unlock given feed */

void glite_jpis_unlockFeed(glite_jp_context_t ctx, char *PS_URL)
{
}

