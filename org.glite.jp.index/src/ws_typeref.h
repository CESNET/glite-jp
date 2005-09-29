#ifndef GLITE_JPIS_TYPEREF_H
#define GLITE_JPIS_TYPEREF_H

int glite_jpis_QueryCondToSoap(struct soap *soap, glite_jp_query_rec_t *in, struct jptype__primaryQuery **out);

void SoapToAttrVal(glite_jp_attrval_t *av, const struct jptype__attrValue *attr);

#endif
