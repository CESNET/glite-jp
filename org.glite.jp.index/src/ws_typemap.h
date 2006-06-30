// XXX: may be glued with org.glite.jp.primary/src/jptype_map.h?

#include "soap_version.h"

#if GSOAP_VERSION >= 20700

#define SYSTEM jptype__attrOrig__SYSTEM
#define USER jptype__attrOrig__USER
#define FILE_ jptype__attrOrig__FILE_

#define EQUAL jptype__queryOp__EQUAL
#define UNEQUAL jptype__queryOp__UNEQUAL
#define LESS jptype__queryOp__LESS
#define GREATER jptype__queryOp__GREATER
#define WITHIN jptype__queryOp__WITHIN
#define EXISTS jptype__queryOp__EXISTS

#else

#define __jpsrv__UpdateJobs __ns1__UpdateJobs
#define __jpsrv__QueryJobs __ns1__QueryJobs
#define __jpsrv__AddFeed __ns1__AddFeed
#define __jpsrv__GetFeeds __ns1__getFeedIDs
#define __jpsrv__DeleteFeed __ns1__DeleteFeed
#define __jpsrv__ServerConfiguration __ns1__ServerConfiguration

#endif

