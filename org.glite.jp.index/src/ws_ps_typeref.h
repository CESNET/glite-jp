#ifndef GLITE_JPIS_TYPEREF_H
#define GLITE_JPIS_TYPEREF_H

int glite_jpis_QueryCondToSoap(struct soap *soap, glite_jp_query_rec_t *in, struct jptype__primaryQuery **out);

void glite_jpis_AttrOrigToSoap(struct soap *soap, const glite_jp_attr_orig_t in, enum jptype__attrOrig **out);

void glite_jpis_SoapToAttrVal(glite_jp_attrval_t *av, const struct jptype__attrValue *attr);

#endif
