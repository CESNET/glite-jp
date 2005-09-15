#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

#include <mysqld_error.h>
#include <errmsg.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "dbis.h"


#define DEFAULTCS	"jpis/@localhost:jpis1"

#define JP_ERR(CTX, CODE, DESC) jp_err((CTX), (CODE), (DESC), __FUNCTION__, __LINE__)
#define MY_ERR(CTX) my_err((CTX), __FUNCTION__, __LINE__)
#define MY_ISOK(CTX, RETRY) my_isok((CTX), __FUNCTION__, __LINE__, (RETRY))
#define MY_ERRSTMT(JPSTMT) my_errstmt((JPSTMT), __FUNCTION__, __LINE__)
#define MY_ISOKSTMT(JPSTMT, RETRY) my_isokstmt((JPSTMT), __FUNCTION__, __LINE__, (RETRY))


struct glite_jpis_db_stmt_s {
	glite_jp_context_t ctx;
	MYSQL_RES *result;
	MYSQL_STMT *stmt;
};


static int jp_err(glite_jp_context_t ctx, int code, const char *desc, const char *source, int line) {
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


static int my_err(glite_jp_context_t ctx, const char *source, int line) {	
	return jp_err(ctx, EIO, mysql_error((MYSQL *)ctx->dbhandle), source, line);
}


static int my_errstmt(glite_jpis_db_stmt_t *jpstmt, const char *source, int line) {	
	return jp_err(jpstmt->ctx, EIO, mysql_stmt_error(jpstmt->stmt), source, line);
}


/*
 * Error handle.
 *
 * \return -1 failed
 * \return  0 retry
 * \return  1 OK
 */
static int my_isok(glite_jp_context_t ctx, const char *source, int line, int *retry) {
	switch (mysql_errno((MYSQL *) ctx->dbhandle)) {
		case 0:
			return 1;
			break;
		case ER_DUP_ENTRY:
			jp_err(ctx, EEXIST, mysql_error((MYSQL *) ctx->dbhandle), source, line);
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
			my_err(ctx, source, line);
			return -1;
			break;
	}
}


/*
 * Error handle.
 *
 * \return -1 failed
 * \return  0 retry
 * \return  1 OK
 */
static int my_isokstmt(glite_jpis_db_stmt_t *jpstmt, const char *source, int line, int *retry) {
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


int glite_jpis_db_connect(glite_jp_context_t ctx, const char *cs) {
	char	*buf = NULL;
	char	*host,*user,*pw,*db; 
	char	*slash,*at,*colon;
	int ret;

	glite_jp_clear_error(ctx);

	if (!cs) cs = DEFAULTCS;

	if ((ctx->dbhandle = (void *) mysql_init(NULL)) == NULL)
		return JP_ERR(ctx, ENOMEM, NULL);

	// TODO: ???
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


void glite_jpis_db_close(glite_jp_context_t ctx) {
	mysql_close((MYSQL *) ctx->dbhandle);
	ctx->dbhandle = NULL;
}


#if 0
int glite_jpis_db_query(glite_jp_context_t ctx, const char *sql, glite_jpis_db_stmt_t **jpstmt) {
	int retry, ret;
	va_list ap;
	void **params;
	unsigned long i, nparams;
	MYSQL_RES *r;

	glite_jp_clear_error(ctx);

	// execute the SQL command
	retry = 1;
	do {
		mysql_query((MYSQL *)ctx->dbhandle, sql);
		ret = MY_ISOK(ctx, &retry);
	} while (ret == 0);
	if (ret == -1) goto failed;

	// result
	r = mysql_store_result((MYSQL *)ctx->dbhandle);
	if (mysql_errno((MYSQL *) ctx->dbhandle)) {
		MY_ERR(ctx);
		goto failed;
	}
	if (jpstmt) {
		*jpstmt = calloc(1, sizeof(glite_jpis_db_stmt_t));
		(*jpstmt)->ctx = ctx;
		(*jpstmt)->result = r;
	} else mysql_free_result(r);

	return mysql_affected_rows((MYSQL *) ctx->dbhandle);

failed:
	return -1;
}


int glite_jpis_db_fetchrow(glite_jpis_db_stmt_t *jpstmt, char ***res, int **lenres) {
	MYSQL_ROW	row;
	glite_jp_context_t	ctx;
	int 		i, nr;
	unsigned long	*len;
	unsigned long    sumlen;
	char *buf;

	ctx = jpstmt->ctx;
	glite_jp_clear_error(ctx);

	*res = NULL;
	*lenres = NULL;

	if (!jpstmt->result) return 0;

	if (!(row = mysql_fetch_row(jpstmt->result))) {
		if (mysql_errno((MYSQL *) ctx->dbhandle)) {
			MY_ERR(ctx);
			return -1;
		}

		return 0;
	}

	// find out
	nr = mysql_num_fields(jpstmt->result);
	len = mysql_fetch_lengths(jpstmt->result);

	// allocate
	if (nr) {
		sumlen = nr;
		for (i = 0; i < nr; i++) sumlen += len[i];
		buf = malloc(sumlen);
	}
	*res = calloc(nr + 1, sizeof(char *));
	if (lenres) {
		if (nr) *lenres = calloc(nr, sizeof(int));
		else *lenres = NULL;
	}

	// copy
	for (i = 0; i < nr; i++) {
		if (lenres) (*lenres)[i] = len[i];
		if (row[i]) {
			(*res)[i] = buf;
			if (len[i]) memcpy((*res)[i], row[i], len[i]);
			((*res)[i])[len[i]] = '\0';
			buf += (len[i] + 1);
		} else (*res)[i] = NULL;
	}
	res[nr] = NULL;

	return nr;
}


void glite_jpis_db_freerow(char **res, int *lenres) {
	if (res) {
		free(res[0]);
		free(res);
		free(lenres);
	}
}


int glite_jpis_db_querycolumns(glite_jpis_db_stmt_t *jpstmt, char **cols) {
	int	i = 0;
	MYSQL_FIELD 	*f;

	while ((f = mysql_fetch_field(jpstmt->result))) cols[i++] = f->name;
	return i == 0;
}
#endif


void glite_jpis_db_freestmt(glite_jpis_db_stmt_t *jpstmt) {
	if (jpstmt->result) mysql_free_result(jpstmt->result);
	if (jpstmt->stmt) mysql_stmt_close(jpstmt->stmt);
	free(jpstmt);
}


void glite_jpis_db_assign_param(MYSQL_BIND *param, enum enum_field_types type, ...) {
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

void glite_jpis_db_assign_result(MYSQL_BIND *param, enum enum_field_types type, my_bool *is_null, ...) {
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


int glite_jpis_db_prepare(glite_jp_context_t ctx, const char *sql, glite_jpis_db_stmt_t **jpstmt, MYSQL_BIND *params, MYSQL_BIND *cols) {
	int ret, retry;

	glite_jp_clear_error(ctx);

	// init
	*jpstmt = malloc(sizeof(struct glite_jpis_db_stmt_s));
	(*jpstmt)->ctx = ctx;
	(*jpstmt)->result = NULL;

	// create the SQL command
	retry = 1;
	do {
		(*jpstmt)->stmt = mysql_stmt_init((MYSQL *)ctx->dbhandle);
		ret = MY_ISOKSTMT(*jpstmt, &retry);
	} while (ret == 0);
	if (ret == -1) goto failed;

	// prepare the SQL command
	retry = 1;
	do {
		mysql_stmt_prepare((*jpstmt)->stmt, sql, strlen(sql));
		ret = MY_ISOKSTMT(*jpstmt, &retry);
	} while (ret == 0);
	if (ret == -1) goto failed;

	// parameters
	if (params) {
		retry = 1;
		do {
			mysql_stmt_bind_param((*jpstmt)->stmt, params);
			ret = MY_ISOKSTMT(*jpstmt, &retry);
		} while (ret == 0);
		if (ret == -1) return -1;
	}

	// results
	if (cols) {
		retry = 1;
		do {
			mysql_stmt_bind_result((*jpstmt)->stmt, cols);
			ret = MY_ISOKSTMT(*jpstmt, &retry);
		} while (ret == 0);
		if (ret == -1) return -1;
	}

	return 0;

failed:
	return ctx->error->code;
}


int glite_jpis_db_execute(glite_jpis_db_stmt_t *jpstmt) {
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


int glite_jpis_db_fetch_stmt(glite_jpis_db_stmt_t *jpstmt) {
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


#ifdef TEST_DBIS
static void print_err(glite_jp_context_t ctx) {
	glite_jp_error_t *e;

	e = ctx->error;
	while (e) {
		printf("%s(%s)\n", e->desc, e->source);
		e = e->reason;
	}
	printf("\n");
}


int main() {
	glite_jp_context_t ctx;
	glite_jpis_db_stmt_t *jpstmt;
	int nr, i;
	char **res;
	int *lenres;

	glite_jp_init_context(&ctx);

	printf("connecting...\n");
	if (glite_jpis_db_connect(ctx, NULL) != 0) goto fail;

#if 0
	// "trio" queries

	printf("selecting...\n");
	if ((glite_jpis_db_execute(ctx, "SELECT * FROM feeds", &jpstmt)) == -1) goto fail;

	printf("fetching...\n");
	while ((nr = glite_jpis_db_fetchrow(jpstmt, &res, &lenres)) > 0) {
		printf("Result: n=%d, res=%p, lenres=%p\n", nr, res, lenres);
		i = 0;
		if (res) while(i < nr) {printf("p=%p(%s), len=%d\n", res[i], res[i], lenres[i]);i++;}
		printf("freeing...\n");
		glite_jpis_db_freerow(res, lenres);
	}
#endif
#if 1
	// param queries
{
	char res_feedid[33];
	long int res_state;
	char res_source[256];
	char res_attrs[1024];
	char res_condition[1024];
	unsigned long res_attrs_length;
	unsigned long res_condition_length;
	long int param_state;
	MYSQL_BIND my_res[5], my_param[1];

	glite_jpis_db_assign_param(my_param+0, MYSQL_TYPE_LONG, &param_state);
	glite_jpis_db_assign_result(my_res+0, MYSQL_TYPE_VAR_STRING, NULL, res_feedid, sizeof(res_feedid), NULL);
	glite_jpis_db_assign_result(my_res+1, MYSQL_TYPE_LONG, NULL, &res_state);
	glite_jpis_db_assign_result(my_res+2, MYSQL_TYPE_VAR_STRING, NULL, res_source, sizeof(res_source), NULL);
	glite_jpis_db_assign_result(my_res+3, MYSQL_TYPE_MEDIUM_BLOB, NULL, res_attrs, sizeof(res_attrs), &res_attrs_length);
	glite_jpis_db_assign_result(my_res+4, MYSQL_TYPE_MEDIUM_BLOB, NULL, res_condition, sizeof(res_condition), &res_condition_length);
	printf("preparing...\n");
	if ((glite_jpis_db_prepare(ctx, "SELECT feedid, state, source, attrs, condition FROM feeds WHERE state = ?", &jpstmt, my_param, my_res)) != 0) goto fail_close;

	printf("executing 1...\n");
	param_state = 1;
	if (glite_jpis_db_execute(jpstmt) == -1) {
		glite_jpis_db_freestmt(jpstmt);
		goto fail_stmtclose;
	}
	printf("fetching...\n");
	while (glite_jpis_db_fetch_stmt(jpstmt) == 0) {
		printf("feedis:%s, state:%ld, source:%s, condition:%s\n", res_feedid, res_state, res_source, res_condition);
	}

	printf("executing 2...\n");
	param_state = 0;
	if (glite_jpis_db_execute(jpstmt) == -1) {
		glite_jpis_db_freestmt(jpstmt);
		goto fail_stmtclose;
	}
	printf("fetching...\n");
	while (glite_jpis_db_fetch_stmt(jpstmt) == 0) {
		printf("feedis:%s, state:%ld, source:%s, condition:%s\n", res_feedid, res_state, res_source, res_condition);
	}
}
#endif
	printf("closing stmt...\n");
	glite_jpis_db_freestmt(jpstmt);
	printf("closing...\n");
	glite_jpis_db_close(ctx);

	glite_jp_free_context(ctx);
	return 0;

fail_stmtclose:
	printf("closing stmt...\n");
	glite_jpis_db_freestmt(jpstmt);
fail_close:
	printf("closing...\n");
	glite_jpis_db_close(ctx);
fail:
	printf("failed\n");
	print_err(ctx);
	glite_jp_free_context(ctx);

	return 1;
}
#endif
