#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include <mysql.h>
#include <mysqld_error.h>
#include <errmsg.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "dbis.h"


#define DEFAULTCS	"jpis/@localhost:jpis1"

#define JP_ERR(CTX, CODE, DESC) jp_err((CTX), (CODE), (DESC), __FUNCTION__, __LINE__)
#define MY_ERR(CTX) my_err((CTX), __FUNCTION__, __LINE__)
#define MY_ISOK(CTX, RETRY) my_isok((CTX), __FUNCTION__, __LINE__, (RETRY))


struct glite_jpis_db_stmt_s {
	glite_jp_context_t ctx;
	MYSQL_RES *result;
};


static int jp_err(glite_jp_context_t ctx, int code, const char *desc, const char *source, int line) {
	glite_jp_error_t err; 

	memset(&err,0,sizeof err); 
	asprintf(&err.source, "%s:%d", source, line);
	err.code = code;
	err.desc = strdup(desc); 

	return glite_jp_stack_error(ctx,&err); 
}


static int my_err(glite_jp_context_t ctx, const char *source, int line) {	
	return jp_err(ctx, EIO, mysql_error((MYSQL *)ctx->dbhandle), source, line);
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


int glite_jpis_db_connect(glite_jp_context_t ctx, const char *cs) {
	char	*buf = NULL;
	char	*host,*user,*pw,*db; 
	char	*slash,*at,*colon;

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
		return JP_ERR(ctx, EINVAL, "Invalid DB connect string");
	}

	*slash = *at = *colon = 0;
	host = at+1;
	user = buf;
	pw = slash+1;
	db = colon+1;

	if (!mysql_real_connect((MYSQL *) ctx->dbhandle,host,user,pw,db,0,NULL,CLIENT_FOUND_ROWS)) {
		free(buf);
		return MY_ERR(ctx);
	}

	free(buf);
	return 0;
}


void glite_jpis_db_close(glite_jp_context_t ctx) {
	mysql_close((MYSQL *) ctx->dbhandle);
	ctx->dbhandle = NULL;
}


int glite_jpis_db_execute(glite_jp_context_t ctx, const char *sql, glite_jpis_db_stmt_t **jpstmt) {
	int retry, ret;
	va_list ap;
	MYSQL_BIND *binds;
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


void glite_jpis_db_freestmt(glite_jpis_db_stmt_t *jpstmt) {
	if (jpstmt->result) mysql_free_result(jpstmt->result);
	free(jpstmt);
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

	glite_jpis_db_freestmt(jpstmt);

	printf("closing...\n");
	glite_jpis_db_close(ctx);
	glite_jp_free_context(ctx);
	return 0;

fail:
	printf("failed\n");
	print_err(ctx);
	glite_jp_free_context(ctx);

	return 1;
}
#endif
