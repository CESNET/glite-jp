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

#ifndef GLITE_JPIS_TYPEREF_H
#define GLITE_JPIS_TYPEREF_H

int glite_jpis_QueryCondToSoap(struct soap *soap, glite_jp_query_rec_t *in, struct jptype__primaryQuery *out);

void glite_jpis_AttrOrigToSoap(struct soap *soap, const glite_jp_attr_orig_t in, enum jptype__attrOrig **out);

void glite_jpis_SoapToAttrVal(glite_jp_attrval_t *av, const struct jptype__attrValue *attr);

#endif
