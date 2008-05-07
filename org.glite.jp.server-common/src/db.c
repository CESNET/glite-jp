#ident "$Header$"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "db.h"


int glite_jp_db_SetError(glite_jp_context_t ctx, const char *source) {
	glite_jp_error_t jperr;
	char *desc;

	memset(&jperr, 0, sizeof jperr);
	if (ctx->dbhandle) {
		jperr.code = glite_lbu_DBError(ctx->dbhandle, NULL, &desc);
		if (jperr.code && source) jperr.source = source;
		jperr.desc = desc;
	} else {
		asprintf(&desc, "DB context isn't created");
		jperr.code = EINVAL;
		jperr.desc = desc;
		jperr.source = __FUNCTION__;
	}
	if (jperr.code) {           
		glite_jp_stack_error(ctx, &jperr);
		free(desc);
	}

	return jperr.code;
}


int glite_jp_db_ExecSQL(glite_jp_context_t ctx, const char *cmd, glite_lbu_Statement *stmt) {
	int num;

	num = glite_lbu_ExecSQL(ctx->dbhandle, cmd, stmt);
	if (num < 0) glite_jp_db_SetError(ctx, __FUNCTION__);

	return num;
}


int glite_jp_db_FetchRow(glite_jp_context_t ctx, glite_lbu_Statement stmt, unsigned int n, unsigned long *lengths, char **results) {
	int num;

	num = glite_lbu_FetchRow(stmt, n, lengths, results);
	if (num < 0) glite_jp_db_SetError(ctx, __FUNCTION__);
	return num;
}


int glite_jp_db_PrepareStmt(glite_jp_context_t ctx, const char *sql, glite_lbu_Statement *stmt) {
	int ret;

	ret = glite_lbu_PrepareStmt(ctx->dbhandle, sql, stmt);
	if (ret != 0) glite_jp_db_SetError(ctx, __FUNCTION__);
	return ret;
}


int glite_jp_db_ExecPreparedStmt(glite_jp_context_t ctx, glite_lbu_Statement stmt, int n,...) {
	va_list ap;
	int ret;

	va_start(ap, n);
	ret = glite_lbu_ExecPreparedStmt_v(stmt, n, ap);
	va_end(ap);
	if (ret < 0) glite_jp_db_SetError(ctx, __FUNCTION__);
	return ret;
}


void glite_jp_db_FreeStmt(glite_lbu_Statement *stmt) {
	glite_lbu_FreeStmt(stmt);
}


int glite_jp_db_Transaction(glite_jp_context_t ctx) {
	int ret;

	ret = glite_lbu_Transaction(ctx->dbhandle);
	if (ret != 0) glite_jp_db_SetError(ctx, __FUNCTION__);

	return ret;
}


int glite_jp_db_Commit(glite_jp_context_t ctx) {
	int ret;

	ret = glite_lbu_Commit(ctx->dbhandle);
	if (ret != 0) glite_jp_db_SetError(ctx, __FUNCTION__);

	return ret;
}


int glite_jp_db_Rollback(glite_jp_context_t ctx) {
	int ret;

	ret = glite_lbu_Rollback(ctx->dbhandle);
	if (ret != 0) glite_jp_db_SetError(ctx, __FUNCTION__);

	return ret;
}
