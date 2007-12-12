#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "glite/lbu/trio.h"
#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "glite/jp/db.h"

extern void reply(int n, char *fmt,...);

#define FTPBE_DEFAULT_DB_CS     "jpps/@localhost:jpps"

static char *user_subj = NULL;
static char *int_prefix = NULL;
static glite_jp_context_t ctx;

static int open_db()
{
	char *db_cs = NULL;

	db_cs = getenv("FTPBE_DB_CS");
	if (!db_cs) db_cs = FTPBE_DEFAULT_DB_CS;

	int_prefix = getenv("FTPBE_INT_PREFIX");
	if (!int_prefix) {
		reply(550, "Internal error: prefix not configured");
		return 0;
	}

	glite_jp_init_context(&ctx);
	if (glite_lbu_InitDBContext(((glite_lbu_DBContext *)&ctx->dbhandle)) != 0) {
		reply(550, "Internal error: backend DB initialization failed");
		return 0;
	}
	if (glite_lbu_DBConnect(ctx->dbhandle, db_cs) != 0) {
		reply(550, "Internal error: backend DB access failed");
		return 0;
	}
	
	return 1;
}

static void close_db()
{
	glite_lbu_DBClose(ctx->dbhandle);
	glite_lbu_FreeDBContext(ctx->dbhandle);
}


int globus_gss_assist_gridmap(char* globus_id, char** mapped_name)
{
	char *logname;

	logname = getenv("GLITE_USER");
	if (logname) {
		*mapped_name = strdup(logname);
		user_subj = strdup(globus_id);
		if (!(*mapped_name) || !user_subj) return 1;
				 
		return 0;
	} else {
		return 1;
		/* 
		 * Note: return value need not follow globus numbering
		 * scheme in ftpd
		 */
	}
}

int globus_gss_assist_userok(char*globus_id, char *account)
{
	char *logname;

	logname = getenv("GLITE_USER");
	if (logname) 
		return strcmp(account,strdup(logname)) ? 1 : 0;
	else
		return 1;
}

int checknoretrieve(char *name)
{
	int result = 1; /* deny access by default */

	char *stmt = NULL;
	int db_retn;
	glite_lbu_Statement db_res;
	char *db_row[1] = { NULL };

	trio_asprintf(&stmt,"select j.owner from jobs j,files f where "
			"f.ext_url='%|Ss%|Ss' and j.jobid=f.jobid",
			int_prefix, name);
	if (!stmt) {
		reply(550, "Internal error: out of memory");
		return 1;
	}

	if (!open_db()) return 1;

	if ((db_retn = glite_jp_db_ExecSQL(ctx, stmt, &db_res)) <= 0) {
		if (db_retn == 0) {
			reply(553, "No such file registered");
		} else {
			reply(550, "Internal error: backend DB access failed");
		}
		goto out;
	}
	
	db_retn = glite_jp_db_FetchRow(ctx, db_res, 1, NULL, db_row);
	if (db_retn != 1) {
		glite_jp_db_FreeStmt(&db_res);
		reply(550, "Internal error: backend DB access failed");
		goto out;
	}
	glite_jp_db_FreeStmt(&db_res);

	if (!strcmp(db_row[0], user_subj)) {
		result = 0;
	} else {
		reply(553, "Permission denied");
	}

out:
	free(db_row[0]);
	close_db();
	free(stmt);
	return result;
}

int upl_check(char *name, uid_t * uid, gid_t * gid, int *f_mode, int *valid)
{
	int result = -1; /* deny access by default */

	char *stmt = NULL;
	int db_retn;
	glite_lbu_Statement db_res;
	char *db_row[1] = { NULL };

	*valid = 0; /* don't used uid & gid */

	trio_asprintf(&stmt,"select state from files "
			"where ext_url='%|Ss%|Ss' and ul_userid='%|Ss'",
			int_prefix, name, user_subj);
	if (!stmt) {
		reply(550, "Internal error: out of memory");
		return -1;
	}

	if (!open_db()) return -1;

	if ((db_retn = glite_jp_db_ExecSQL(ctx, stmt, &db_res)) <= 0) {
		if (db_retn == 0) {
			reply(553, "No such upload in progress");
		} else {
			reply(550, "Internal error: backend DB access failed");
		}
		goto out;
	}
	
	db_retn = glite_jp_db_FetchRow(ctx, db_res, 1, NULL, db_row);
	if (db_retn != 1) {
		glite_jp_db_FreeStmt(&db_res);
		reply(550, "Internal error: backend DB access failed");
		goto out;
	}
	glite_jp_db_FreeStmt(&db_res);

	if (!strcmp(db_row[0], "uploading")) {
		result = 1;
	} else {
		reply(553, "Permission denied");
	}

out:
	free(db_row[0]);
	close_db();
	free(stmt);
	return result;
}

int del_check(char *name)
{
	reply(553, "Deleting files not supported");
	return 0;
}

int rename(const char *f, const char * t)
{
	errno = EPERM;
	return -1;
}

FILE *ftpd_popen(char *program, char *type, int closestderr)
{
	errno = EPERM;
	return NULL;
}
