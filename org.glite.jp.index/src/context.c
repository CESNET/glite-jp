#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <globus_common.h>

#include "conf.h"
#include "context.h"


int glite_jpis_init_context(glite_jpis_context_t *isctx, glite_jp_context_t jpctx, glite_jp_is_conf *conf) {
	char hname[512];

	if ((*isctx = calloc(sizeof(**isctx), 1)) != NULL) {
		(*isctx)->jpctx = jpctx;
		(*isctx)->conf = conf;
		globus_libc_gethostname(hname, sizeof hname);
		asprintf(&(*isctx)->hname, "https://%s:%s", hname, (conf && conf->port) ? conf->port : GLITE_JPIS_DEFAULT_PORT_STR);
		return 0;
	} else return ENOMEM;
}


void glite_jpis_free_context(glite_jpis_context_t ctx) {
	free(ctx->hname);
	free(ctx);
}
