#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <glite/security/glite_gss.h>

#include "conf.h"
#include "context.h"


int glite_jpis_init_context(glite_jpis_context_t *isctx, glite_jp_context_t jpctx, glite_jp_is_conf *conf) {
	char hname[512];
	char *op_args;

	if ((*isctx = calloc(sizeof(**isctx), 1)) != NULL) {
		(*isctx)->jpctx = jpctx;
		(*isctx)->conf = conf;
		edg_wll_gss_gethostname(hname, sizeof hname);
		asprintf(&(*isctx)->hname, "https://%s:%s", hname, (conf && conf->port) ? conf->port : GLITE_JPIS_DEFAULT_PORT_STR);

		op_args = (*isctx)->op_args;
		op_args[GLITE_JP_QUERYOP_WITHIN] = 2;
		op_args[GLITE_JP_QUERYOP_UNDEF] = 0;
		op_args[GLITE_JP_QUERYOP_EQUAL] = 1;
		op_args[GLITE_JP_QUERYOP_LESS] = 1;
		op_args[GLITE_JP_QUERYOP_GREATER] = 1;
		op_args[GLITE_JP_QUERYOP_EXISTS] = 0;
		return 0;
	} else return ENOMEM;
}


void glite_jpis_free_context(glite_jpis_context_t ctx) {
	if (!ctx) return;
	free(ctx->hname);
	free(ctx);
}
