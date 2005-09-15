#ifndef GLITE_JP_IS_DB_H
#define GLITE_JP_IS_DB_H

#include <mysql.h>

#include <glite/jp/types.h>
#include <glite/jp/context.h>

typedef struct glite_jpis_db_stmt_s glite_jpis_db_stmt_t;

int glite_jpis_db_connect(glite_jp_context_t ctx, const char *cs);
void glite_jpis_db_close(glite_jp_context_t ctx);
#if 0
int glite_jpis_db_query(glite_jp_context_t ctx, const char *sql, glite_jpis_db_stmt_t **stmt);
int glite_jpis_db_fetchrow(glite_jpis_db_stmt_t *jpstmt, char ***res, int **lenres);
void glite_jpis_db_freerow(char **res, int *lenres);
int glite_jpis_db_querycolumns(glite_jpis_db_stmt_t *jpstmt, char **cols);
#endif
void glite_jpis_db_freestmt(glite_jpis_db_stmt_t *jpstmt);

void glite_jpis_db_assign_param(MYSQL_BIND *param, enum enum_field_types type, ...);
void glite_jpis_db_assign_result(MYSQL_BIND *param, enum enum_field_types type, my_bool *is_null, ...);
int glite_jpis_db_prepare(glite_jp_context_t ctx, const char *sql, glite_jpis_db_stmt_t **jpstmt, MYSQL_BIND *params, MYSQL_BIND *cols);
int glite_jpis_db_execute(glite_jpis_db_stmt_t *jpstmt);
int glite_jpis_db_fetch_stmt(glite_jpis_db_stmt_t *jpstmt);

#endif
