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

