#ifndef _DB_H
#define _DB_H

#ident "$Header$"
/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners/ for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


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
int glite_jp_db_Transaction(glite_jp_context_t ctx);
int glite_jp_db_Commit(glite_jp_context_t ctx);
int glite_jp_db_Rollback(glite_jp_context_t ctx);

#ifdef __cplusplus
}
#endif

#endif
