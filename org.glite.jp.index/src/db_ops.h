#ident "$Header$"

#ifndef _DB_OPS_H
#define _DB_OPS_H

int glite_jpis_lockUninitializedFeed(char **PS_URL);
void glite_jpis_feedInit(char *PS_URL, char *feedId, time_t feedExpires);
void glite_jpis_unlockFeed(char *PS_URL);

#endif
