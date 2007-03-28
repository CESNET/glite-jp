#ifndef GLITE_JPIS_IS_TYPEREF_H
#define GLITE_JPIS_IS_TYPEREF_H

#include "jpis_H.h"

void glite_jpis_SoapToQueryOp(const enum jptype__queryOp in, glite_jp_queryop_t *out);
void glite_jpis_SoapToAttrOrig(struct soap *soap, const enum jptype__attrOrig *in, glite_jp_attr_orig_t *out);
int glite_jpis_SoapToQueryConds(struct soap *soap, int size, struct jptype__indexQuery **in, glite_jp_query_rec_t ***out);
int glite_jpis_SoapToPrimaryQueryConds(struct soap *soap, int size, GLITE_SECURITY_GSOAP_LIST_TYPE(jptype, primaryQuery) in, glite_jp_query_rec_t ***out);

#endif
