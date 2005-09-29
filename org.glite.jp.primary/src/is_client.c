#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "glite/jp/types.h"

#include "feed.h"
#include "is_client.h"
/* FIXME
#include "jpis_H.h"
#include "jpis_.nsmap"
*/

int glite_jpps_single_feed(
		glite_jp_context_t ctx,
		const char *destination,
		const char *job,
		glite_jp_attrval_t const *attrs
)
{
	/* TODO: really call JP Index server (via interlogger) */
	printf("feed to %s, job %s\n",destination,job);

/* FIXME */
#if 0
	if (soap_call_jpsrv__UpdateJobs(ctx->other_soap,destination,"",
		/* FIXME: feedId */ "",
		/* FIXME: UpdateJobsData */ NULL,
		0,
		NULL
	)) fprintf(stderr,"UpdateJobs: %s %s\n",ctx->other_soap->fault->faultcode,
		ctx->other_soap->fault->faultstring);

#endif
	return 0;
}

int glite_jpps_multi_feed(
		glite_jp_context_t ctx,
		int njobs,
		const char *dest,
		char **jobs,
		glite_jp_attrval_t **attrs)
{
	int	i,j;

	printf("multi_feed: %s\n",dest);
	for (i=0; i<njobs; i++) {
		puts(jobs[i]);
		for (j=0; attrs[i][j].name; j++)
			printf("%s = %s\n",attrs[i][j].name,attrs[i][j].value);
		putchar(10);
	}
}

