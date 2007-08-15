#ident "$Header$"

#include "mysql.h"	// MySql header file
#include "mysqld_error.h"
#include "errmsg.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <limits.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "db.h"

#define DEFAULTCS	"jpps/@localhost:jpps1"
#define GLITE_JP_LB_MYSQL_VERSION 40018

static int  my_err(glite_jp_context_t ctx, char *function)
{	
	glite_jp_error_t err; 

  	glite_jp_clear_error(ctx); 
	memset(&err,0,sizeof err); 
	err.source = function;
	err.code = EIO; /* XXX */
	err.desc = mysql_error((MYSQL *) ctx->dbhandle); 
	return glite_jp_stack_error(ctx,&err); 
}

struct _glite_jp_db_stmt_t {
	MYSQL_RES		*result;
	glite_jp_context_t	ctx;
};

int glite_jp_db_connect(glite_jp_context_t ctx,char *cs)
{
	char	*buf = NULL;
	char	*host,*user,*pw,*db; 
	char	*slash,*at,*colon;

	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	if (!cs) cs = DEFAULTCS;

	if (!(ctx->dbhandle = (void *) mysql_init(NULL))) {
		err.code = ENOMEM;
		return glite_jp_stack_error(ctx,&err);
	}

	mysql_options(ctx->dbhandle, MYSQL_READ_DEFAULT_FILE, "my");

	host = user = pw = db = NULL;

	buf = strdup(cs);
	slash = strchr(buf,'/');
	at = strrchr(buf,'@');
	colon = strrchr(buf,':');

	if (!slash || !at || !colon) {
		free(buf);
		err.code = EINVAL;
		err.desc = "Invalid DB connect string";
		return glite_jp_stack_error(ctx,&err);
	}

	*slash = *at = *colon = 0;
	host = at+1;
	user = buf;
	pw = slash+1;
	db = colon+1;

	if (!mysql_real_connect((MYSQL *) ctx->dbhandle,host,user,pw,db,0,NULL,CLIENT_FOUND_ROWS)) {
		free(buf);
		return my_err(ctx, __FUNCTION__);
	}

	free(buf);
	return 0;
}

void glite_jp_db_close(glite_jp_context_t ctx)
{
	mysql_close((MYSQL *) ctx->dbhandle);
	ctx->dbhandle = NULL;
}

int glite_jp_db_execstmt(glite_jp_context_t ctx,char *txt,glite_jp_db_stmt_t *stmt)
{
	int	merr;
	int	retry_nr = 0;
	int	do_reconnect = 0;

	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	if (stmt) {
		*stmt = NULL;
	}

	while (retry_nr == 0 || do_reconnect) {
		do_reconnect = 0;
		if (mysql_query((MYSQL *) ctx->dbhandle,txt)) {
			/* error occured */
			switch (merr = mysql_errno((MYSQL *) ctx->dbhandle)) {
				case 0:
					break;
				case ER_DUP_ENTRY: 
					err.code = EEXIST;
					err.desc = mysql_error((MYSQL *) ctx->dbhandle);
					glite_jp_stack_error(ctx,&err);
					return -1;
					break;
				case CR_SERVER_GONE_ERROR:
				case CR_SERVER_LOST:
					if (retry_nr <= 0) 
						do_reconnect = 1;
					break;
				default:
					my_err(ctx, __FUNCTION__);
					return -1;
					break;
			}
		}
		retry_nr++;
	}

	if (stmt) {
		*stmt = malloc(sizeof(**stmt));
		if (!*stmt) {
			err.code = ENOMEM;
			glite_jp_stack_error(ctx,&err);
			return -1;
		}
		memset(*stmt,0,sizeof(**stmt));
		(**stmt).ctx = ctx;
		(**stmt).result = mysql_store_result((MYSQL *) ctx->dbhandle);
		if (!(**stmt).result) {
			if (mysql_errno((MYSQL *) ctx->dbhandle)) {
				my_err(ctx, __FUNCTION__);
				return -1;
			}
		}
	} else {
		MYSQL_RES	*r = mysql_store_result((MYSQL *) ctx->dbhandle);
		mysql_free_result(r);
	}
	
	return mysql_affected_rows((MYSQL *) ctx->dbhandle);
}

int glite_jp_db_fetchrow(glite_jp_db_stmt_t stmt,char **res)
{
	MYSQL_ROW	row;
	glite_jp_context_t	ctx = stmt->ctx;
	int 		nr,i;
	unsigned long	*len;

	glite_jp_clear_error(ctx);

	if (!stmt->result) return 0;

	if (!(row = mysql_fetch_row(stmt->result))) {
		if (mysql_errno((MYSQL *) ctx->dbhandle)) {
			my_err(ctx, __FUNCTION__);
			return -1;
		} else return 0;
	}

	nr = mysql_num_fields(stmt->result);
	len = mysql_fetch_lengths(stmt->result);
	for (i=0; i<nr; i++) res[i] = len[i] ? strdup(row[i]) : strdup("");

	return nr;
}

int glite_jp_db_querycolumns(glite_jp_db_stmt_t stmt,char **cols)
{
	int	i = 0;
	MYSQL_FIELD 	*f;

	while ((f = mysql_fetch_field(stmt->result))) cols[i++] = f->name;
	return i == 0;
}

void glite_jp_db_freestmt(glite_jp_db_stmt_t *stmt)
{
	if (*stmt) {
		if ((**stmt).result) mysql_free_result((**stmt).result);
		free(*stmt);
		*stmt = NULL;
	}
}


char *glite_jp_db_timetodb(time_t t)
{
	struct tm	*tm = gmtime(&t);
	char	tbuf[256];

	/* XXX: the very end of our days */
	if (!tm && t == (time_t) LONG_MAX) return strdup("9999-12-31 23:59:59");

	sprintf(tbuf,"'%4d-%02d-%02d %02d:%02d:%02d'",tm->tm_year+1900,tm->tm_mon+1,
		tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	
	return strdup(tbuf);
}

time_t glite_jp_db_dbtotime(char *t)
{
	struct tm	tm;

	memset(&tm,0,sizeof(tm));
	setenv("TZ","UTC",1); tzset();
	sscanf(t,"%4d-%02d-%02d %02d:%02d:%02d",
		&tm.tm_year,&tm.tm_mon,&tm.tm_mday,
		&tm.tm_hour,&tm.tm_min,&tm.tm_sec);
	tm.tm_year -= 1900;
	tm.tm_mon--;

	return mktime(&tm);
}

int glite_jp_db_dbcheckversion(glite_jp_context_t ctx)
{
	MYSQL	*m = (MYSQL *) ctx->dbhandle;
	const   char *ver_s = mysql_get_server_info(m);
	int	major,minor,sub,version;

	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	if (!ver_s || 3 != sscanf(ver_s,"%d.%d.%d",&major,&minor,&sub)) {
		err.code = EINVAL;
		err.desc = "problem checking MySQL version";
		return glite_jp_stack_error(ctx,&err);
	}

	version = 10000*major + 100*minor + sub;

	if (version < GLITE_JP_LB_MYSQL_VERSION) {
		char	msg[300];

		snprintf(msg,sizeof msg,"Your MySQL version is %d. At least %d required.",version, GLITE_JP_LB_MYSQL_VERSION);
		err.code = EINVAL;
		err.desc = msg;
		return glite_jp_stack_error(ctx,&err);
	}

	return 0;
}
