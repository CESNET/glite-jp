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

#ifndef GLITE_JP_BACKEND_H
#define GLITE_JP_BACKEND_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "feed.h"

int glite_jppsbe_init(
	glite_jp_context_t ctx,
	int argc,
	char *argv[]
);

int glite_jppsbe_init_slave(
	glite_jp_context_t ctx
);

int glite_jppsbe_register_job(	
	glite_jp_context_t ctx,
	const char *job,
	const char *owner
);

int glite_jppsbe_start_upload(
	glite_jp_context_t ctx,
	const char *job,
	const char *class,	/* must be filesystem-friendly */
	const char *name,	/* optional name within the class */
	const char *content_type,
	char **destination_out,
	time_t *commit_before_inout
);

int glite_jppsbe_commit_upload(
	glite_jp_context_t ctx,
	const char *destination
);

int glite_jppsbe_get_names(
	glite_jp_context_t ctx,
	const char *job,
	const char *class,
	char	***names_out
);

int glite_jppsbe_destination_info(
	glite_jp_context_t ctx,
	const char *destination,
	char **job_out,
	char **class_out,
	char **name_out
);

int glite_jppsbe_get_job_url(
	glite_jp_context_t ctx,
	const char *job,
	const char *class,
	const char *name,	/* optional within class */
	char **url_out
);

int glite_jppsbe_open_file(
	glite_jp_context_t ctx,
	const char *job,
	const char *class,
	const char *name,	/* optional within class */
	int mode,
	void **handle_out
);

int glite_jppsbe_close_file(
	glite_jp_context_t ctx,
	void *handle
);

int glite_jppsbe_file_attrs(
	glite_jp_context_t ctx,
	void *handle,
	struct stat *buf
);

int glite_jppsbe_pread(
	glite_jp_context_t ctx,
	void *handle,
	void *buf,
	size_t nbytes,
	off_t offset,
	ssize_t *nbytes_ret
);

int glite_jppsbe_pwrite(
	glite_jp_context_t ctx,
	void *handle,
	void *buf,
	size_t nbytes,
	off_t offset
);

int glite_jppsbe_append(
	glite_jp_context_t ctx,
	void *handle,
	void *buf,
	size_t nbytes
);

int glite_jppsbe_is_metadata(
	glite_jp_context_t ctx,
	const char *attr
);

int glite_jppsbe_get_job_metadata(
	glite_jp_context_t ctx,
	const char *job,
	glite_jp_attrval_t attrs_inout[]
);

int glite_jppsbe_query(
	glite_jp_context_t ctx,
	const glite_jp_query_rec_t query[],
	char *attrs[],
	void *arg,
	int (*callback)(
		glite_jp_context_t ctx,
		const char *job,
		const glite_jp_attrval_t metadata[],
		void *arg
	)
);

/** mark the job as sent to this feed */
int glite_jppsbe_set_fed(
	glite_jp_context_t ctx,
	const char *feed,
	const char *job
);

/** check whether the job has been already sent to this feed */
int glite_jppsbe_check_fed(
	glite_jp_context_t ctx,
	const char *feed,
	const char *job,
	int *result
);

/** store the feed to database */
int glite_jppsbe_store_feed(
	glite_jp_context_t ctx,
	struct jpfeed *feed
);

/** purge expired feeds */
int glite_jppsbe_purge_feeds(
	glite_jp_context_t ctx
);

/** read stored feed into context */
int glite_jppsbe_read_feeds(
	glite_jp_context_t ctx
);

#endif /* GLITE_JP_BACKEND_H */
