#ident "$Header$"

#include <mysql.h>
#include <mysqld_error.h>
#include <errmsg.h>

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


#define GLITE_JP_DB_MYSQL_VERSION 40102

#if !defined(MYSQL_VERSION_ID) || MYSQL_VERSION_ID < GLITE_JP_DB_MYSQL_VERSION
#error required MySQL version 4.1.2
#endif

#define JP_ERR(CTX, CODE, DESC) jp_err((CTX), (CODE), (DESC), __FUNCTION__, __LINE__)
#define MY_ERR(CTX) my_err((CTX), __FUNCTION__, __LINE__)
#define MY_ERRSTMT(JPSTMT) my_errstmt((JPSTMT), __FUNCTION__, __LINE__)
#define MY_ISOKSTMT(JPSTMT, RETRY) my_isokstmt((JPSTMT), __FUNCTION__, __LINE__, (RETRY))


typedef struct {
	int		n;
	MYSQL_BIND	params[1];
} params_t;

struct _glite_jp_db_stmt_t {
	glite_jp_context_t	ctx;
	MYSQL_RES		*result;
	MYSQL_STMT		*stmt;
	params_t		*params, *results;
};


static int glite_to_mysql_type[] = {
	MYSQL_TYPE_NULL,
	MYSQL_TYPE_TINY,
	MYSQL_TYPE_LONG,
	MYSQL_TYPE_TINY_BLOB,
	MYSQL_TYPE_TINY_BLOB,
	MYSQL_TYPE_BLOB,
	MYSQL_TYPE_BLOB,
	MYSQL_TYPE_MEDIUM_BLOB,
	MYSQL_TYPE_MEDIUM_BLOB,
	MYSQL_TYPE_LONG_BLOB,
	MYSQL_TYPE_LONG_BLOB,
	MYSQL_TYPE_VAR_STRING,
	MYSQL_TYPE_STRING,
	MYSQL_TYPE_DATE,
	MYSQL_TYPE_TIME,
	MYSQL_TYPE_DATETIME,
	MYSQL_TYPE_TIMESTAMP,
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

	// needed for SQL result parameters
	assert(sizeof(int) >= sizeof(my_bool));

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
		glite_jp_db_close(ctx);
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
		glite_jp_db_close(ctx);
		return ret;
	}
	free(buf);

	if ((ret = glite_jp_db_dbcheckversion(ctx)) != 0) {
		glite_jp_db_close(ctx);
		return ret;
	}

	return 0;
}


void glite_jp_db_close(glite_jp_context_t ctx)
{
	if (ctx->dbhandle) {
		mysql_close((MYSQL *) ctx->dbhandle);
		ctx->dbhandle = NULL;
	}
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
		if ((*stmt)->params) glite_jp_db_destroy_params((*stmt)->params);
		if ((*stmt)->results) glite_jp_db_destroy_results((*stmt)->results);
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

	if (version < GLITE_JP_DB_MYSQL_VERSION) {
		char	msg[300];

		return JP_ERR(ctx, EINVAL, msg);
	}

	return 0;
}


void glite_jp_db_create_params(void **params, int n, ...) {
	params_t *myparams;
	MYSQL_BIND *myparam;
	MYSQL_TIME **mytime;
	int i;
	va_list ap;
	glite_jp_db_type_t type;

	myparams = calloc(n, sizeof(params_t) + (n - 1) * sizeof(MYSQL_BIND));
	va_start(ap, n);

	for (i = 0; i < n; i++) {
		myparam = myparams->params + i;
		type = va_arg(ap, glite_jp_db_type_t);

		switch (type) {
		case GLITE_JP_DB_TYPE_TINYINT:
			myparam->buffer = va_arg(ap, char *);
			break;

		case GLITE_JP_DB_TYPE_INT:
			myparam->buffer = va_arg(ap, long int *);
			break;

		case GLITE_JP_DB_TYPE_TINYBLOB:
		case GLITE_JP_DB_TYPE_TINYTEXT:
		case GLITE_JP_DB_TYPE_BLOB:
		case GLITE_JP_DB_TYPE_TEXT:
		case GLITE_JP_DB_TYPE_MEDIUMBLOB:
		case GLITE_JP_DB_TYPE_MEDIUMTEXT:
		case GLITE_JP_DB_TYPE_LONGBLOB:
		case GLITE_JP_DB_TYPE_LONGTEXT:
			myparam->buffer = va_arg(ap, void *);
			myparam->length = va_arg(ap, unsigned long *);
			break;

		case GLITE_JP_DB_TYPE_VARCHAR:
		case GLITE_JP_DB_TYPE_CHAR:
			myparam->buffer = va_arg(ap, char *);
			myparam->length = va_arg(ap, unsigned long *);
			break;

		case GLITE_JP_DB_TYPE_DATE:
		case GLITE_JP_DB_TYPE_TIME:
		case GLITE_JP_DB_TYPE_DATETIME:
		case GLITE_JP_DB_TYPE_TIMESTAMP:
			mytime = (MYSQL_TIME **)va_arg(ap, void **);
			*mytime = calloc(1, sizeof(MYSQL_TIME));
			myparam->buffer = *mytime;
			break;

		case GLITE_JP_DB_TYPE_NULL:
			break;

		default:
			assert("unimplemented parameter assign" == NULL);
			break;
		}
		myparam->buffer_type = glite_to_mysql_type[type];
	}
	myparams->n = n;

	va_end(ap);
	*params = myparams;
}


void glite_jp_db_create_results(void **results, int n, ...) {
	params_t *myresults;
	MYSQL_BIND *myresult;
	MYSQL_TIME **mytime;
	va_list ap;
	int i;
	glite_jp_db_type_t type;
	int *is_null;

	myresults = calloc(n, sizeof(params_t) + (n - 1) * sizeof(MYSQL_BIND));
	va_start(ap, n);

	for (i = 0; i < n; i++) {
		myresult = myresults->params + i;
		type = va_arg(ap, glite_jp_db_type_t);
		is_null = va_arg(ap, int *);
		myresult->is_null = (my_bool *)is_null;
		if (is_null) *is_null = 0;

		switch(type) {
		case GLITE_JP_DB_TYPE_TINYINT:
			myresult->buffer = va_arg(ap, char *);
			myresult->buffer_length = sizeof(char);
			break;

		case GLITE_JP_DB_TYPE_INT:
			myresult->buffer = va_arg(ap, long int *);
			myresult->buffer_length = sizeof(long int);
			break;

		case GLITE_JP_DB_TYPE_TINYBLOB:
		case GLITE_JP_DB_TYPE_TINYTEXT:
		case GLITE_JP_DB_TYPE_BLOB:
		case GLITE_JP_DB_TYPE_TEXT:
		case GLITE_JP_DB_TYPE_MEDIUMBLOB:
		case GLITE_JP_DB_TYPE_MEDIUMTEXT:
		case GLITE_JP_DB_TYPE_LONGBLOB:
		case GLITE_JP_DB_TYPE_LONGTEXT:
			myresult->buffer = va_arg(ap, void *);
			myresult->buffer_length = va_arg(ap, unsigned long);
			myresult->length = va_arg(ap, unsigned long *);
			break;

		case GLITE_JP_DB_TYPE_VARCHAR:
		case GLITE_JP_DB_TYPE_CHAR:
			myresult->buffer = va_arg(ap, char *);
			myresult->buffer_length = va_arg(ap, unsigned long);
			myresult->length = va_arg(ap, unsigned long *);
			break;

		case GLITE_JP_DB_TYPE_DATE:
		case GLITE_JP_DB_TYPE_TIME:
		case GLITE_JP_DB_TYPE_DATETIME:
		case GLITE_JP_DB_TYPE_TIMESTAMP:
			mytime = (MYSQL_TIME **)va_arg(ap, void **);
			*mytime = calloc(1, sizeof(MYSQL_TIME));
			myresult->buffer = *mytime;
			break;

		default:
			assert("unimplemented result assign" == NULL);
		}
		myresult->buffer_type = glite_to_mysql_type[type];
		if (myresult->buffer && myresult->buffer_length) memset(myresult->buffer, 0, myresult->buffer_length);
	}
	myresults->n = n;

	va_end(ap);
	*results = myresults;
}


static void glite_jp_db_destroy_respam(params_t *params) {
	MYSQL_BIND *myparam;
	int i;
	enum enum_field_types type;

	for (i = 0; i < params->n; i++) {
		myparam = params->params + i;
		type = myparam->buffer_type;
		if (type == MYSQL_TYPE_DATE || type == MYSQL_TYPE_TIME || type == MYSQL_TYPE_DATETIME || type == MYSQL_TYPE_TIMESTAMP) {
			free(myparam->buffer);
			myparam->buffer = NULL;
		}
	}
	free(params);
}


void glite_jp_db_destroy_params(void *params) {
	glite_jp_db_destroy_respam(params);
}


void glite_jp_db_destroy_results(void *results) {
	glite_jp_db_destroy_respam(results);
}


void glite_jp_db_set_time(void *buffer, const time_t time) {
	MYSQL_TIME *mybuffer;
	struct tm tm;

	mybuffer = (MYSQL_TIME *)buffer;
	gmtime_r(&time, &tm);
	mybuffer->year = tm.tm_year + 1900;
	mybuffer->month = tm.tm_mon + 1;
	mybuffer->day = tm.tm_mday;
	mybuffer->hour = tm.tm_hour;
	mybuffer->minute = tm.tm_min;
	mybuffer->second = tm.tm_sec;
}


time_t glite_jp_db_get_time(const void *buffer) {
	MYSQL_TIME *mybuffer;
	struct tm tm;

	mybuffer = (MYSQL_TIME *)buffer;
	memset(&tm, 0, sizeof(tm));
	tm.tm_year = mybuffer->year - 1900;
	tm.tm_mon = mybuffer->month - 1;
	tm.tm_mday = mybuffer->day;
	tm.tm_hour = mybuffer->hour;
	tm.tm_min = mybuffer->minute;
	tm.tm_sec = mybuffer->second;

	return mktime(&tm);
}


int glite_jp_db_rebind(glite_jp_db_stmt_t jpstmt, void *params, void *cols) {
	if (jpstmt->params) {
		glite_jp_db_destroy_params(jpstmt->params);
		jpstmt->params = NULL;
	}
	if (jpstmt->results) {
		glite_jp_db_destroy_results(jpstmt->results);
		jpstmt->results = NULL;
	}
	if (params) {
		jpstmt->params = (params_t *)params;
		if (mysql_stmt_bind_param(jpstmt->stmt, jpstmt->params->params) != 0) return MY_ERRSTMT(jpstmt);
	}
	if (cols) {
		jpstmt->results = (params_t *)cols;
		if (mysql_stmt_bind_result(jpstmt->stmt, jpstmt->results->params) != 0) return MY_ERRSTMT(jpstmt);
	}

	return 0;
}


int glite_jp_db_prepare(glite_jp_context_t ctx, const char *sql, glite_jp_db_stmt_t *jpstmt, void *params, void *cols) {
	int ret, retry;

	glite_jp_clear_error(ctx);

	// init
	*jpstmt = calloc(1, sizeof(struct _glite_jp_db_stmt_t));
	(*jpstmt)->ctx = ctx;

	// create the SQL command
	if (((*jpstmt)->stmt = mysql_stmt_init((MYSQL *)ctx->dbhandle)) == NULL) {
		ret = MY_ERRSTMT(*jpstmt);
		goto failed;
	}

	// prepare the SQL command
	retry = 1;
	do {
		mysql_stmt_prepare((*jpstmt)->stmt, sql, strlen(sql));
		ret = MY_ISOKSTMT(*jpstmt, &retry);
	} while (ret == 0);
	if (ret == -1) goto failed;

	// parameters and results
	if ((ret = glite_jp_db_rebind(*jpstmt, params, cols)) != 0) goto failed;

	return 0;

failed:
	if (params) glite_jp_db_destroy_params(params);
	if (cols) glite_jp_db_destroy_params(cols);
	glite_jp_db_freestmt(jpstmt);
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


long int glite_jp_db_lastid(glite_jp_db_stmt_t jpstmt) {
	my_ulonglong i;

	glite_jp_clear_error(jpstmt->ctx);
	i = mysql_stmt_insert_id(jpstmt->stmt);
	assert(i < ((unsigned long int)-1) >> 1);
	return (long int)i;
}
