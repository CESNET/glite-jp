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

#ifndef GLITE_JPIS_IS_TYPEREF_H
#define GLITE_JPIS_IS_TYPEREF_H

#include "jp_H.h"

void glite_jpis_SoapToQueryOp(const enum jptype__queryOp in, glite_jp_queryop_t *out);
void glite_jpis_SoapToAttrOrig(const enum jptype__attrOrig *in, glite_jp_attr_orig_t *out);
#if 0
int glite_jpis_SoapToQueryConds(int size, struct jptype__indexQuery **in, glite_jp_query_rec_t ***out);
#endif
int glite_jpis_SoapToPrimaryQueryConds(int size, GLITE_SECURITY_GSOAP_LIST_TYPE(jptype, primaryQuery) in, glite_jp_query_rec_t **out);

#endif
