#ident "$Header$"

#include <time.h>

/* Find first unitialized feed, lock it and return URL of corresponding PS 
 *
 * Return value:
 *      0      - OK
 *      ENOENT - no more feeds to initialize
 *      ENOLCK - error during locking */

int glite_jpis_lockUninitializedFeed(char **PS_URL)
{
	return 0;
}


/* Store feed ID and expiration time returned by PS for locked feed. */

void glite_jpis_feedInit(char *PS_URL, char *feedId, time_t feedExpires)
{
}

/* Unlock given feed */

void glite_jpis_unlockFeed(char *PS_URL)
{
}

