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

#ifndef GLITE_JP_IS_CONTEXT_H
#define GLITE_JP_IS_CONTEXT_H

#include <glite/jp/types.h>
#include <glite/jp/context.h>
#include <glite/jp/db.h>
#include "conf.h"


typedef struct _glite_jpis_context {
	glite_jp_context_t jpctx;
	glite_jp_is_conf *conf;
	glite_lbu_Statement select_unlocked_feed_stmt, lock_feed_stmt, init_feed_stmt, unlock_feed_stmt, select_info_feed_stmt, update_state_feed_stmt, select_info_attrs_indexed, select_jobid_stmt, select_user_stmt, insert_job_stmt, insert_user_stmt;

	char *hname;

	char op_args[GLITE_JP_QUERYOP__LAST];
} *glite_jpis_context_t;

typedef struct _slave_data_t{
        glite_jpis_context_t ctx;
        glite_jp_is_conf *conf;
        struct soap *soap;
} slave_data_t;

int glite_jpis_init_context(glite_jpis_context_t *isctx, glite_jp_context_t jpctx, glite_jp_is_conf *conf);
void glite_jpis_free_context(glite_jpis_context_t ctx);
#endif
