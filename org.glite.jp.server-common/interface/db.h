#ifndef _DB_H
#define _DB_H

#ident "$Header$"

#include "glite/jp/types.h"
#include "glite/lbu/db.h"

#ifdef __cplusplus
extern "C" {
#endif

int glite_jp_db_SetError(glite_jp_context_t ctx, const char *source);
int glite_jp_db_ExecSQL(glite_jp_context_t ctx, const char *cmd, glite_lbu_Statement *stmt);
int glite_jp_db_FetchRow(glite_jp_context_t ctx, glite_lbu_Statement stmt, unsigned int n, unsigned long *lengths, char **results);
int glite_jp_db_PrepareStmt(glite_jp_context_t ctx, const char *sql, glite_lbu_Statement *stmt);
int glite_jp_db_ExecPreparedStmt(glite_jp_context_t ctx, glite_lbu_Statement stmt, int n,...);
void glite_jp_db_FreeStmt(glite_lbu_Statement *stmt);

#ifdef __cplusplus
}
#endif

#endif
