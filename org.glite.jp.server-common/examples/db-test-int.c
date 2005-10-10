#include <stdio.h>
#include <stdlib.h>

#include "db.h"


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
	glite_jp_db_stmt_t jpstmt;

	glite_jp_init_context(&ctx);

	printf("connecting...\n");
	if (glite_jp_db_connect(ctx, "jpis/@localhost:jpis1") != 0) goto fail;

	// "trio" queries
{
	int nr, i;
	char **res;

	printf("selecting...\n");
	if ((glite_jp_db_execstmt(ctx, "SELECT uniqueid, feedid, state, source, condition FROM feeds", &jpstmt)) == -1) goto fail;

	printf("fetching...\n");
	res = calloc(4, sizeof(char *));
	while ((nr = glite_jp_db_fetchrow(jpstmt, res)) > 0) {
		printf("Result: n=%d, res=%p\n", nr, res);
		i = 0;
		if (res) while(i < nr) {printf("p=%p(%s)\n", res[i], res[i]);free(res[i]);i++;}
	}
	free(res);
	printf("closing stmt...\n");
	glite_jp_db_freestmt(&jpstmt);
}

	// param queries
{
	char res_feedid[33];
	long int res_state;
	char res_source[256];
	char res_condition[1024];
	unsigned long res_condition_length;
	long int param_state;

	void *my_res, *my_param;

	glite_jp_db_create_params(&my_param, 1, GLITE_JP_DB_TYPE_INT, &param_state);
	glite_jp_db_create_results(&my_res, 4,
		GLITE_JP_DB_TYPE_VARCHAR, NULL, res_feedid, sizeof(res_feedid), NULL,
		GLITE_JP_DB_TYPE_INT, NULL, &res_state,
		GLITE_JP_DB_TYPE_VARCHAR, NULL, res_source, sizeof(res_source), NULL,
		GLITE_JP_DB_TYPE_MEDIUMBLOB, NULL, res_condition, sizeof(res_condition), &res_condition_length
	);
	printf("preparing...\n");
	if ((glite_jp_db_prepare(ctx, "SELECT feedid, state, source, condition FROM feeds WHERE state = ?", &jpstmt, my_param, my_res)) != 0) goto fail_close;

	param_state = 1;
	printf("executing state %ld...\n", param_state);
	if (glite_jp_db_execute(jpstmt) == -1) {
		glite_jp_db_freestmt(&jpstmt);
		goto fail_stmtclose;
	}
	printf("fetching...\n");
	while (glite_jp_db_fetch(jpstmt) == 0) {
		printf("feedid:%s, state:%ld, source:%s, condition:%s\n", res_feedid, res_state, res_source, res_condition);
	}

	param_state = 2;
	printf("executing state %ld...\n", param_state);
	if (glite_jp_db_execute(jpstmt) == -1) {
		glite_jp_db_freestmt(&jpstmt);
		goto fail_stmtclose;
	}
	printf("fetching...\n");
	while (glite_jp_db_fetch(jpstmt) == 0) {
		printf("feedid:%s, state:%ld, source:%s, condition:%s\n", res_feedid, res_state, res_source, res_condition);
	}
}

	printf("closing stmt...\n");
	glite_jp_db_freestmt(&jpstmt);
	printf("closing...\n");
	glite_jp_db_close(ctx);

	glite_jp_free_context(ctx);
	return 0;

fail_stmtclose:
	printf("closing stmt...\n");
	glite_jp_db_freestmt(&jpstmt);
fail_close:
	printf("closing...\n");
	glite_jp_db_close(ctx);
fail:
	printf("failed\n");
	print_err(ctx);
	glite_jp_free_context(ctx);

	return 1;
}
