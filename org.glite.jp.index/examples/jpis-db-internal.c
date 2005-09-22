#include <stddef.h>

#include <glite/jp/db.h>

#include "db_ops.h"


#define CS "jpis/@localhost:jpis1"


static print_err(glite_jp_context_t ctx) {
	glite_jp_error_t *e;

	e = ctx->error;
	while(e) {
		printf("%s(%s)\n", e->desc, e->source);
		e = e->reason;
	}
	printf("\n");
}


int main(int argc, char *argv[]) {
	glite_jp_context_t ctx;
	glite_jp_is_conf *conf;

	glite_jp_init_context(&ctx);
	if (glite_jp_db_connect(ctx, CS) != 0) goto fail;

	printf("dropping...\n");
	if (glite_jpis_dropDatabase(ctx) != 0) goto faildb;

	printf("initializing...\n");
	if (glite_jp_get_conf(argc, argv, NULL, &conf) != 0) goto faildb;
	if (glite_jpis_initDatabase(ctx, conf) != 0) goto failconf;

	glite_jp_free_conf(conf);
	glite_jp_db_close(ctx);
	glite_jp_free_context(ctx);

	return 0;

failconf:
	glite_jp_free_conf(conf);
faildb:
	glite_jp_db_close(ctx);
fail:
	printf("failed\n");
	print_err(ctx);
	glite_jp_free_context(ctx);
	return 1;
}
