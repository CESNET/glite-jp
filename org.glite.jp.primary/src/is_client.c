#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "glite/jp/types.h"

#include "feed.h"
#include "jpis_H.h"

int glite_jpps_single_feed(
		glite_jp_context_t ctx,
		const char *destination,
		const char *job,
		const glite_jp_attrval_t attrs[]
)
{
	/* TODO: really call JP Index server (via interlogger) */
	printf("feed to %s, job %s\n",destination,job);

	/* FIXME: check fault */
	soap_call_jpsrv__UpdateJobs(ctx->other_soap,destination,"",


	return 0;
}
