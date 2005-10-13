#ident "$Header$"

#ifndef _DB_OPS_H
#define _DB_OPS_H


#include <glite/jp/types.h>
#include <glite/jp/context.h>
#include "context.h"


#define GLITE_JP_IS_DEFAULTCS "jpis/@localhost:jpis1"

#define GLITE_JP_IS_STATE_HIST 1
#define GLITE_JP_IS_STATE_CONT 2
#define GLITE_JP_IS_STATE_DONE 4
#define GLITE_JP_IS_STATE_ERROR 8
#define GLITE_JP_IS_STATE_ERROR_STR "8"


char *glite_jpis_attr_name2id(const char *name);

int glite_jpis_initDatabase(glite_jpis_context_t ctx);
int glite_jpis_dropDatabase(glite_jpis_context_t ctx);

int glite_jpis_init_db(glite_jpis_context_t isctx);
void glite_jpis_free_db(glite_jpis_context_t ctx);

int glite_jpis_lockUninitializedFeed(glite_jpis_context_t ctx, long int *uinqueid, char **PS_URL);
int glite_jpis_initFeed(glite_jpis_context_t ctx, long int uniqueid, char *feedId, time_t feedExpires);
int glite_jpis_unlockFeed(glite_jpis_context_t ctx, long int uniqueid);
int glite_jpis_tryReconnectFeed(glite_jpis_context_t ctx, long int uniqueid, time_t reconn_time);

int glite_jpis_insertAttrVal(glite_jpis_context_t ctx, const char *jobid, glite_jp_attrval_t *av);

int glite_jpis_lazyInsertJob(glite_jpis_context_t ctx, const char *feedid, const char *jobid, const char *owner);

#endif
