#ident "$Header$"

#ifndef _DB_OPS_H
#define _DB_OPS_H

#include <glite/jp/types.h>
#include <glite/jp/context.h>
#include "conf.h"


#define GLITE_JP_IS_STATE_HIST 1
#define GLITE_JP_IS_STATE_CONT 2


int glite_jpis_initDatabase(glite_jp_context_t ctx, glite_jp_is_conf *conf);
int glite_jpis_dropDatabase(glite_jp_context_t ctx);
int glite_jpis_lockUninitializedFeed(glite_jp_context_t ctx, char **PS_URL);
void glite_jpis_feedInit(glite_jp_context_t ctx, char *PS_URL, char *feedId, time_t feedExpires);
void glite_jpis_unlockFeed(glite_jp_context_t ctx, char *PS_URL);

#endif
