#ident "$Header$"

#ifndef _SOAP_PS_CALLS_H
#define _SOAP_PS_CALLS_H

#include "context.h"
#include "conf.h"

int MyFeedIndex(struct soap *soap, glite_jpis_context_t ctx, long int uniqueid, const char *dest);
int MyFeedRefresh(struct soap *soap, glite_jpis_context_t ctx, long int uniqueid, const char *dest, int status, const char *feedid);

#endif
