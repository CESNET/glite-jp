#ident "$Header$"

#include "mysql.h"
#include "mysqld_error.h"
#include "errmsg.h"

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <stdarg.h>
#include <assert.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "db.h"

#define GLITE_JP_LB_MYSQL_VERSION 40018

#define JP_ERR(CTX, CODE, DESC) jp_err((CTX), (CODE), (DESC), __FUNCTION__, __LINE__)
#define MY_ERR(CTX) my_err((CTX), __FUNCTION__, __LINE__)
#define MY_ERRSTMT(JPSTMT) my_errstmt((JPSTMT), __FUNCTION__, __LINE__)
#define MY_ISOKSTMT(JPSTMT, RETRY) my_isokstmt((JPSTMT), __FUNCTION__, __LINE__, (RETRY))


struct _glite_jp_db_stmt_t {
	glite_jp_context_t	ctx;
	MYSQL_RES		*result;
	MYSQL_STMT		*stmt;
};


static int jp_err(glite_jp_context_t ctx, int code, const char *desc, const char *source, int line)
{
	glite_jp_error_t err; 
	char *fullsource;
	int ret;

	asprintf(&fullsource, "%s:%d", source, line);
	memset(&err,0,sizeof err); 
	err.code = code;
	err.source = fullsource;
	err.desc = desc; 

	ret = glite_jp_stack_error(ctx,&err); 
	free(fullsource);
	return ret;
}


static int my_err(glite_jp_context_t ctx, const char *source, int line)
{	
	return jp_err(ctx, EIO, mysql_error((MYSQL *)ctx->dbhandle), source, line);
}


static int my_errstmt(glite_jp_db_stmt_t jpstmt, const char *source, int line) {	
	return jp_err(jpstmt->ctx, EIO, mysql_stmt_error(jpstmt->stmt), source, line);
}


/*
 * Error handle.
 *
 * \return -1 failed
 * \return  0 retry
 * \return  1 OK
 */
static int my_isokstmt(glite_jp_db_stmt_t jpstmt, const char *source, int line, int *retry) {
	switch (mysql_stmt_errno(jpstmt->stmt)) {
		case 0:
			return 1;
			break;
		case ER_DUP_ENTRY:
			jp_err(jpstmt->ctx, EEXIST, mysql_stmt_error(jpstmt->stmt), source, line);
			return -1;
			break;
		case CR_SERVER_LOST:
			if (*retry > 0) {
				(*retry)--;
				return 0;
			} else
				return -1;
			break;
		default:
			my_errstmt(jpstmt, source, line);
			return -1;
			break;
	}
}


int glite_jp_db_connect(glite_jp_context_t ctx,const char *cs)
{
	char	*buf = NULL;
	char	*host,*user,*pw,*db; 
	char	*slash,*at,*colon;
	int      ret;

	glite_jp_clear_error(ctx);

	if (!cs) return JP_ERR(ctx, EINVAL, "connect string not specified");

	if (!(ctx->dbhandle = (void *) mysql_init(NULL))) return JP_ERR(ctx, ENOMEM, NULL);

	mysql_options(ctx->dbhandle, MYSQL_READ_DEFAULT_FILE, "my");

	host = user = pw = db = NULL;

	buf = strdup(cs);
	slash = strchr(buf,'/');
	at = strrchr(buf,'@');
	colon = strrchr(buf,':');

	if (!slash || !at || !colon) {
		free(buf);
		mysql_close((MYSQL *)ctx->dbhandle);
		return JP_ERR(ctx, EINVAL, "Invalid DB connect string");
	}

	*slash = *at = *colon = 0;
	host = at+1;
	user = buf;
	pw = slash+1;
	db = colon+1;

	if (!mysql_real_connect((MYSQL *) ctx->dbhandle,host,user,pw,db,0,NULL,CLIENT_FOUND_ROWS)) {
		free(buf);
		ret = MY_ERR(ctx);
		mysql_close((MYSQL *)ctx->dbhandle);
		return ret;
	}

	free(buf);
	return 0;
}


void glite_jp_db_close(glite_jp_context_t ctx)
{
	mysql_close((MYSQL *) ctx->dbhandle);
	ctx->dbhandle = NULL;
}


int glite_jp_db_execstmt(glite_jp_context_t ctx,const char *txt,glite_jp_db_stmt_t *stmt)
{
	int	merr;
	int	retry_nr = 0;
	int	do_reconnect = 0;

	glite_jp_clear_error(ctx);

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
					JP_ERR(ctx, EEXIST, mysql_error((MYSQL *) ctx->dbhandle));
					return -1;
					break;
				case CR_SERVER_LOST:
					if (retry_nr <= 0) 
						do_reconnect = 1;
					break;
				default:
					MY_ERR(ctx);
					return -1;
					break;
			}
		}
		retry_nr++;
	}

	if (stmt) {
		*stmt = malloc(sizeof(**stmt));
		if (!*stmt) {
			JP_ERR(ctx, ENOMEM, NULL);
			return -1;
		}
		memset(*stmt,0,sizeof(**stmt));
		(**stmt).ctx = ctx;
		(**stmt).result = mysql_store_result((MYSQL *) ctx->dbhandle);
		if (!(**stmt).result) {
			if (mysql_errno((MYSQL *) ctx->dbhandle)) {
				MY_ERR(ctx);
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
			MY_ERR(ctx);
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
		if ((*stmt)->stmt) mysql_stmt_close((*stmt)->stmt);
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


time_t glite_jp_db_dbtotime(const char *t)
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

	glite_jp_clear_error(ctx);

	if (!ver_s || 3 != sscanf(ver_s,"%d.%d.%d",&major,&minor,&sub)) {
		return JP_ERR(ctx, EINVAL, "problem checking MySQL version");
	}

	version = 10000*major + 100*minor + sub;

	if (version < GLITE_JP_LB_MYSQL_VERSION) {
		char	msg[300];

		return JP_ERR(ctx, EINVAL, msg);
	}

	return 0;
}


void glite_jp_db_assign_param(MYSQL_BIND *param, enum enum_field_types type, ...) {
	va_list ap;

	memset(param, 0, sizeof(*param));
	param->buffer_type = type;

	va_start(ap, type);

	switch (type) {
	case MYSQL_TYPE_TINY:
		param->buffer = va_arg(ap, char *);
		break;

	case MYSQL_TYPE_LONG:
		param->buffer = va_arg(ap, long int *);
		break;

	case MYSQL_TYPE_TINY_BLOB:
	case MYSQL_TYPE_MEDIUM_BLOB:
	case MYSQL_TYPE_LONG_BLOB:
	case MYSQL_TYPE_BLOB:
		param->buffer = va_arg(ap, void *);
		param->length = va_arg(ap, unsigned long *);
		break;

	case MYSQL_TYPE_VAR_STRING:
	case MYSQL_TYPE_STRING:
		param->buffer = va_arg(ap, char *);
		param->length = va_arg(ap, unsigned long *);
		break;

	case MYSQL_TYPE_NULL:
		break;

	default:
		assert("unimplemented parameter assign" == NULL);
		break;
	}

	va_end(ap);
}

void glite_jp_db_assign_result(MYSQL_BIND *param, enum enum_field_types type, my_bool *is_null, ...) {
	va_list ap;

	memset(param, 0, sizeof(*param));
	param->buffer_type = type;
	param->is_null = is_null;

	va_start(ap, is_null);

	switch(type) {
	case MYSQL_TYPE_TINY:
		param->buffer = va_arg(ap, char *);
		param->buffer_length = sizeof(char);
		break;

	case MYSQL_TYPE_LONG:
		param->buffer = va_arg(ap, long int *);
		param->buffer_length = sizeof(long int);
		break;

	case MYSQL_TYPE_TINY_BLOB:
	case MYSQL_TYPE_MEDIUM_BLOB:
	case MYSQL_TYPE_LONG_BLOB:
	case MYSQL_TYPE_BLOB:
		param->buffer = va_arg(ap, void *);
		param->buffer_length = va_arg(ap, unsigned long);
		param->length = va_arg(ap, unsigned long *);
		break;

	case MYSQL_TYPE_VAR_STRING:
	case MYSQL_TYPE_STRING:
		param->buffer = va_arg(ap, char *);
		param->buffer_length = va_arg(ap, unsigned long);
		param->length = va_arg(ap, unsigned long *);
		break;

	default:
		assert("unimplemented result assign" == NULL);
	}
	if (param->buffer && param->buffer_length) memset(param->buffer, 0, param->buffer_length);

	va_end(ap);
}


int glite_jp_db_prepare(glite_jp_context_t ctx, const char *sql, glite_jp_db_stmt_t *jpstmt, MYSQL_BIND *params, MYSQL_BIND *cols) {
	int ret, retry;

	glite_jp_clear_error(ctx);

	// init
	*jpstmt = malloc(sizeof(struct _glite_jp_db_stmt_t));
	(*jpstmt)->ctx = ctx;
	(*jpstmt)->result = NULL;

	// create the SQL command
	if (((*jpstmt)->stmt = mysql_stmt_init((MYSQL *)ctx->dbhandle)) == NULL)
		return MY_ERRSTMT(*jpstmt);

	// prepare the SQL command
	retry = 1;
	do {
		mysql_stmt_prepare((*jpstmt)->stmt, sql, strlen(sql));
		ret = MY_ISOKSTMT(*jpstmt, &retry);
	} while (ret == 0);
	if (ret == -1) goto failed;

	// parameters
	if (params) {
		if (mysql_stmt_bind_param((*jpstmt)->stmt, params) != 0)
			return MY_ERRSTMT(*jpstmt);
	}

	// results
	if (cols) {
		if (mysql_stmt_bind_result((*jpstmt)->stmt, cols) != 0)
			return MY_ERRSTMT(*jpstmt);
	}

	return 0;

failed:
	return ctx->error->code;
}


int glite_jp_db_execute(glite_jp_db_stmt_t jpstmt) {
	glite_jp_context_t ctx;
	int ret, retry;

	ctx = jpstmt->ctx;
	glite_jp_clear_error(ctx);

	// run
	retry = 1;
	do {
		mysql_stmt_execute(jpstmt->stmt);
		ret = MY_ISOKSTMT(jpstmt, &retry);
	} while (ret == 0);
	if (ret == -1) goto failed;

	// result
	mysql_stmt_store_result(jpstmt->stmt);
	if (mysql_stmt_errno(jpstmt->stmt)) {
		MY_ERRSTMT(jpstmt);
		goto failed;
	}

	return mysql_stmt_affected_rows(jpstmt->stmt);

failed:
	return -1;
}


int glite_jp_db_fetch(glite_jp_db_stmt_t jpstmt) {
	int ret, retry;

	glite_jp_clear_error(jpstmt->ctx);

	retry = 1;
	do {
		switch(mysql_stmt_fetch(jpstmt->stmt)) {
		case 0: ret = 1; break;
		case 1: ret = MY_ISOKSTMT(jpstmt, &retry); break;
		case MYSQL_NO_DATA: JP_ERR(jpstmt->ctx, ENODATA, "no more rows"); ret = -1; break;
		default: JP_ERR(jpstmt->ctx, EIO, "other fetch error"); ret = -1; break;
		}
	} while (ret == 0);
	if (ret == -1) goto failed;

	return 0;

failed:
	return jpstmt->ctx->error->code;
}
