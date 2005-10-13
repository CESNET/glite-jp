#ifndef GLITE_JP_IS_CONTEXT_H
#define GLITE_JP_IS_CONTEXT_H

#include <glite/jp/types.h>
#include <glite/jp/context.h>
#include <glite/jp/db.h>
#include "conf.h"


typedef struct _glite_jpis_context {
	glite_jp_context_t jpctx;
	glite_jp_is_conf *conf;
	glite_jp_db_stmt_t select_unlocked_feed_stmt, lock_feed_stmt, init_feed_stmt, unlock_feed_stmt, select_info_feed_stmt, update_state_feed_stmt, update_error_feed_stmt, select_info_attrs_indexed, select_jobid_stmt, insert_job_stmt;
	long int param_uniqueid, param_state;
	char param_feedid[33], param_ps[256], param_indexed[256], param_jobid[33], param_dg_jobid[256], param_ownerid[33];
	unsigned long param_ps_len, param_feedid_len, param_indexed_len, param_jobid_len, param_dg_jobid_len, param_ownerid_len;
	void *param_expires;

	char *hname;
} *glite_jpis_context_t;

typedef struct _slave_data_t{
        glite_jpis_context_t ctx;
        glite_jp_is_conf *conf;
        struct soap *soap;
} slave_data_t;

int glite_jpis_init_context(glite_jpis_context_t *isctx, glite_jp_context_t jpctx, glite_jp_is_conf *conf);
void glite_jpis_free_context(glite_jpis_context_t ctx);
#endif
