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

static enum jptype__attrOrig jp2s_origin(glite_jp_attr_orig_t o)
{
	switch (o) {
		case GLITE_JP_ATTR_ORIG_SYSTEM: return jptype__attrOrig__SYSTEM;
		case GLITE_JP_ATTR_ORIG_USER: return jptype__attrOrig__USER;
		case GLITE_JP_ATTR_ORIG_FILE: return jptype__attrOrig__FILE_;
		default: abort(); /* XXX */
	}
}

static int jp2s_attrValues(
	struct soap *soap,
	glite_jp_attrval_t *in,
	GLITE_SECURITY_GSOAP_LIST_TYPE(jptype, attrValue) *outp,
	int freeit)
{
	GLITE_SECURITY_GSOAP_LIST_TYPE(jptype, attrValue)	out;
	struct jptype__attrValue	a;
	int	i,cnt;

	for (cnt=0; in[cnt].name; cnt++);
	       
	GLITE_SECURITY_GSOAP_LIST_CREATE0(soap, out, cnt, struct jptype__attrValue, cnt);
	for (i=0; in[i].name; i++) {
		memset(&a, 0, sizeof a);
		a.name = soap_strdup(soap,in[i].name); 
		if (freeit) free(in[i].name);
		a.value = soap_malloc(soap,sizeof *a.value);
		memset(a.value, 0, sizeof *a.value);
		if (in[i].binary) {
			GSOAP_SETBLOB(a.value, soap_malloc(soap,sizeof *GSOAP_BLOB(a.value)));
			memset(GSOAP_BLOB(a.value),0,sizeof *GSOAP_BLOB(a.value));
			GSOAP_BLOB(a.value)->__ptr = soap_malloc(soap,in[i].size);
			GSOAP_BLOB(a.value)->__size = in[i].size;
			memcpy(GSOAP_BLOB(a.value)->__ptr,in[i].value,in[i].size);
		}
		else {
			GSOAP_SETSTRING(a.value, soap_strdup(soap,in[i].value));
		}

		if (freeit) free(in[i].value);
		a.origin = jp2s_origin(in[i].origin);
		a.originDetail = in[i].origin_detail ? soap_strdup(soap,in[i].origin_detail) : NULL;
		if (freeit) free(in[i].origin_detail);
		a.timestamp = in[i].timestamp;

		memcpy(GLITE_SECURITY_GSOAP_LIST_GET(out, i), &a, sizeof a);
	}
	if (freeit) free(in);

	*outp = out;
	return cnt;
}

static void attrValues_free(
	struct soap *soap,
	GLITE_SECURITY_GSOAP_LIST_TYPE(jptype, attrValue) a,
	int	na)
{
	int	i;
	struct jptype__attrValue *ai;

	for (i=0; i<na; i++) {
		ai = GLITE_SECURITY_GSOAP_LIST_GET(a, i);
		if (GSOAP_ISSTRING(ai->value) && GSOAP_STRING(ai->value)) soap_dealloc(soap,GSOAP_STRING(ai->value));
		if (GSOAP_ISBLOB(ai->value) && GSOAP_BLOB(ai->value)) {
			soap_dealloc(soap,GSOAP_BLOB(ai->value)->__ptr);
			soap_dealloc(soap,GSOAP_BLOB(ai->value));
		}
		soap_dealloc(soap,ai->value);
		if (ai->originDetail) soap_dealloc(soap,ai->originDetail);
	}
	GLITE_SECURITY_GSOAP_LIST_DESTROY0(soap, a, na);
}
