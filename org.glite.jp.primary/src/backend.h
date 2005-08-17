#ifndef __GLITE_JP_BACKEND
#define __GLITE_JP_BACKEND

#include <sys/types.h>
#include <unistd.h>

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
	const glite_jp_attrval_t metadata[],
	int (*callback)(
		glite_jp_context_t ctx,
		const char *job,
		const glite_jp_attrval_t metadata[]
	)
);

#endif
