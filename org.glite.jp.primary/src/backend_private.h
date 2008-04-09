#ifndef GLITE_JP_BACKEND_PRIVATE_H
#define GLITE_JP_BACKEND_PRIVATE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "feed.h"

#include "glite/jp/backend.h"

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

#endif /* GLITE_JP_BACKEND_PRIVATE_H */
