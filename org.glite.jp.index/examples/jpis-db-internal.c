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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <glite/jp/db.h>

#include "db_ops.h"


static void print_err(glite_jp_context_t ctx) {
	glite_jp_error_t *e;

	e = ctx->error;
	while(e) {
		printf("%s (%s)\n", e->desc, e->source);
		e = e->reason;
	}
	printf("\n");
}


int glite_jpis_db_queries_serialize(void **blob, size_t *len, glite_jp_query_rec_t **queries);
int glite_jpis_db_queries_deserialize(glite_jp_query_rec_t ***queries, void *blob, size_t blob_size);

int main(int argc, char *argv[]) {
#if 1
	glite_jp_context_t jpctx;
	glite_jp_is_conf *conf;
	glite_jpis_context_t isctx;
	int ret;
	long int uniqueid;
	char *ps, *feedid;

	jpctx = NULL;
	isctx = NULL;
	conf = NULL;
	glite_jp_init_context(&jpctx);
	if (glite_jp_get_conf(argc, argv, NULL, &conf) != 0) goto fail;
	if (glite_jpis_init_context(&isctx, jpctx, conf) != 0) goto fail;
	if (glite_jpis_init_db(isctx) != 0) goto fail;

	printf("dropping...\n");
	if (glite_jpis_dropDatabase(isctx) != 0) goto faildb;

	printf("initializing...\n");
	if (glite_jpis_initDatabase(isctx) != 0) goto faildb;

	printf("locking...\n");
	do {
		if ((ret = glite_jpis_lockUninitializedFeed(isctx, &uniqueid, &ps)) == ENOLCK) goto faildb;
		if (ret == 0) {
			printf("locked: uniqueid=%li, ps=%s\n", uniqueid, ps);
			free(ps);

			asprintf(&feedid, "feed://%lu", uniqueid + 3);
			if (glite_jpis_initFeed(isctx, uniqueid, feedid, (time_t)10000) != 0) {
				free(feedid);
				goto faildb;
			}
			free(feedid);

			if (glite_jpis_unlockFeed(isctx, uniqueid) != 0) goto faildb;
		}
	} while (ret == 0);

	if (glite_jpis_tryReconnectFeed(isctx, uniqueid, time(NULL) + 10) != 0) goto faildb;

	glite_jpis_free_db(isctx);
	glite_jpis_free_context(isctx);
	glite_jp_free_conf(conf);
	glite_jp_free_context(jpctx);

	return 0;

faildb:
	glite_jpis_free_db(isctx);
fail:
	printf("failed\n");
	glite_jpis_free_context(isctx);
	glite_jp_free_conf(conf);
	if (jpctx) {
		print_err(jpctx);
		glite_jp_free_context(jpctx);
	}

	return 1;
#endif
#if 0
	glite_jp_context_t ctx;
	glite_jp_is_conf *conf;
	void *blob;
	size_t len;
	int ret, i;
	glite_jp_query_rec_t **queries;

	ret = 0;
	glite_jp_init_context(&ctx);

	if (glite_jp_get_conf(argc, argv, NULL, &conf) != 0) goto fail_ctx;
	if ((ret = glite_jpis_db_queries_serialize(&blob, &len, conf->feeds[0]->query)) != 0) goto fail;

	if (write(1, blob, len) != len) {
		ret = errno;
		free(blob);
		goto fail;
	}

	if ((ret = glite_jpis_db_queries_deserialize(&queries, blob, len)) != 0) goto fail_blob;
	i = 0;
	while (queries[i] && queries[i]->attr) {
		printf("query: attr=%s, op=%d, value=%s, value2=%s, bin=%d\n", queries[i]->attr, queries[i]->op, queries[i]->value, queries[i]->value2, queries[i]->binary);
		free(queries[i]->attr);
		free(queries[i]->value);
		free(queries[i]->value2);
		free(queries[i]);
		i++;
	}
	free(queries);

	free(blob);
	glite_jp_free_context(ctx);
	return 0;

fail_blob:
	free(blob);
fail:
	fprintf(stderr, "fail: %s\n", strerror(ret));
fail_ctx:
	glite_jp_free_context(ctx);
	return 1;
#endif
}
