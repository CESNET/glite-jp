#ident "$Header$"

#ifndef _DB_OPS_H
#define _DB_OPS_H

#include <glite/jp/types.h>
#include <glite/jp/context.h>
#include <glite/jp/db.h>
#include "conf.h"


#define GLITE_JP_IS_DEFAULTCS "jpis/@localhost:jpis1"

#define GLITE_JP_IS_STATE_HIST 1
#define GLITE_JP_IS_STATE_CONT 2
#define GLITE_JP_IS_STATE_DONE 4


typedef struct _glite_jpis_context {
	glite_jp_context_t jpctx;
	glite_jp_db_stmt_t select_unlocked_feed_stmt, lock_feed_stmt, init_feed_stmt, unlock_feed_stmt, select_info_feed_stmt, update_state_feed_stmt;
	long int param_uniqueid, param_state;
	char param_feedid[33], param_ps[256];
	unsigned long param_ps_len, param_feedid_len;
	MYSQL_TIME param_expires;
} *glite_jpis_context_t;

char *glite_jpis_attr_name2id(const char *name);

int glite_jpis_initDatabase(glite_jp_context_t ctx, glite_jp_is_conf *conf);
int glite_jpis_dropDatabase(glite_jp_context_t ctx);

int glite_jpis_init_context(glite_jpis_context_t *isctx, glite_jp_context_t jpctx);
int glite_jpis_free_context(glite_jpis_context_t ctx);

int glite_jpis_lockUninitializedFeed(glite_jpis_context_t ctx, long int *uinqueid, char **PS_URL);
int glite_jpis_initFeed(glite_jpis_context_t ctx, long int uniqueid, char *feedId, time_t feedExpires);
int glite_jpis_unlockFeed(glite_jpis_context_t ctx, long int uniqueid);

int glite_jpis_insertAttrVal(glite_jpis_context_t ctx, const char *jobid, glite_jp_attrval_t *av);

#endif
