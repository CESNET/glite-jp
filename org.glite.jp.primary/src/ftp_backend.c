#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "backend.h"

int glite_jppsbe_init(
	glite_jp_context_t ctx,
	int *argc,
	char *argv[]
)
{
}

int glite_jppsbe_init_slave(
	glite_jp_context_t ctx
)
{
}

int glite_jppsbe_register_job(	
	glite_jp_context_t ctx,
	const char *job,
	const char *owner
)
{
	glite_jp_clear_error(ctx);
	puts(__FUNCTION__);
	return 0;
}

int glite_jppsbe_start_upload(
	glite_jp_context_t ctx,
	const char *job,
	glite_jp_fileclass_t class,
	const char *content_type,
	char **destination_out,
	time_t *commit_before_inout
)
{
	glite_jp_clear_error(ctx);
	puts(__FUNCTION__);
	return 0;
}

int glite_jppsbe_commit_upload(
	glite_jp_context_t ctx,
	const char *destination
)
{
	glite_jp_clear_error(ctx);
	puts(__FUNCTION__);
	return 0;
}

int glite_jppsbe_destination_info(
	glite_jp_context_t ctx,
	const char *destination,
	char **job,
	glite_jp_fileclass_t *class
)
{
	glite_jp_clear_error(ctx);
	puts(__FUNCTION__);
	return 0;
}


int glite_jppsbe_get_job_url(
	glite_jp_context_t ctx,
	const char *job,
	glite_jp_fileclass_t class,
	char **url_out
)
{
	glite_jp_clear_error(ctx);
	puts(__FUNCTION__);
	return 0;
}

int glite_jppsbe_open_file(
	glite_jp_context_t ctx,
	const char *job,
	glite_jp_fileclass_t class,
	int mode,
	void **handle_out
)
{
	glite_jp_clear_error(ctx);
	puts(__FUNCTION__);
	return 0;
}

int glite_jppsbe_close_file(
	glite_jp_context_t ctx,
	void *handle
)
{
	glite_jp_clear_error(ctx);
	puts(__FUNCTION__);
	return 0;
}

int glite_jppsbe_pread(
	glite_jp_context_t ctx,
	void *handle,
	void *buf,
	size_t nbytes,
	off_t offset
)
{
	glite_jp_clear_error(ctx);
	puts(__FUNCTION__);
	return 0;
}

int glite_jppsbe_pwrite(
	glite_jp_context_t ctx,
	void *handle,
	void *buf,
	size_t nbytes,
	off_t offset
)
{
	glite_jp_clear_error(ctx);
	puts(__FUNCTION__);
	return 0;
}

int glite_jppsbe_append(
	glite_jp_context_t ctx,
	void *handle,
	void *buf,
	size_t nbytes
)
{
	glite_jp_clear_error(ctx);
	puts(__FUNCTION__);
	return 0;
}

int glite_jppsbe_get_job_metadata(
	glite_jp_context_t ctx,
	const char *job,
	glite_jp_attrval_t attrs_inout[]
)
{
	glite_jp_clear_error(ctx);
	puts(__FUNCTION__);
	return 0;
}

int glite_jppsbe_query(
	glite_jp_context_t ctx,
	const glite_jp_query_rec_t query[],
	const glite_jp_attrval_t metadata[],
	int (*callback)(
		glite_jp_context_t ctx,
		const char *job,
		const glite_jp_attrval_t metadata[]
	)
)
{
	glite_jp_clear_error(ctx);
	puts(__FUNCTION__);
	return 0;
}
