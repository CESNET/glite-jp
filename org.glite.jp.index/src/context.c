#include <stdlib.h>
#include <errno.h>

#include "conf.h"
#include "context.h"


int glite_jpis_init_context(glite_jpis_context_t *isctx, glite_jp_context_t jpctx, glite_jp_is_conf *conf) {
	if ((*isctx = calloc(sizeof(**isctx), 1)) != NULL) {
		(*isctx)->jpctx = jpctx;
		(*isctx)->conf = conf;
		return 0;
	} else return ENOMEM;
}


void glite_jpis_free_context(glite_jpis_context_t ctx) {
	free(ctx);
}
