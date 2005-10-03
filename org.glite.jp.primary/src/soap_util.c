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
	struct jptype__attrValue ***outp,
	int freeit)
{
	struct jptype__attrValue **out;
	int	i,cnt;

	for (cnt=0; in[cnt].name; cnt++);
	       
	out = soap_malloc(soap,cnt * sizeof *out);
	for (i=0; in[i].name; i++) {
		struct jptype__attrValue	*a = soap_malloc(soap,sizeof *a);
		out[i] = a;

		a->name = soap_strdup(soap,in[i].name); 
		if (freeit) free(in[i].name);
		a->value = soap_malloc(soap,sizeof *a->value);
		if (in[i].binary) {
			a->value->blob = soap_malloc(soap,sizeof *a->value->blob);
			memset(a->value->blob,0,sizeof *a->value->blob);
			a->value->blob->__ptr = soap_malloc(soap,in[i].size);
			a->value->blob->__size = in[i].size;
			memcpy(a->value->blob->__ptr,in[i].value,in[i].size);

			a->value->string = NULL;
		}
		else {
			a->value->string = soap_strdup(soap,in[i].value);
			a->value->blob = NULL;
		}
		if (freeit) free(in[i].value);
		a->origin = jp2s_origin(in[i].origin);
		a->originDetail = in[i].origin_detail ? soap_strdup(soap,in[i].origin_detail) : NULL;
		if (freeit) free(in[i].origin_detail);
		a->timestamp = in[i].timestamp;
	}
	if (freeit) free(in);

	*outp = out;
	return cnt;
}

static void attrValues_free(
	struct soap *soap,
	struct jptype__attrValue **a,
	int	na)
{
	int	i;

	for (i=0; i<na; i++) {
		soap_dealloc(soap,a[i]->name);
		if (a[i]->value->string) soap_dealloc(soap,a[i]->value->string);
		if (a[i]->value->blob) {
			soap_dealloc(soap,a[i]->value->blob->__ptr);
			soap_dealloc(soap,a[i]->value->blob);
		}
		soap_dealloc(soap,a[i]->value);
		if (a[i]->originDetail) soap_dealloc(soap,a[i]->originDetail);
		soap_dealloc(soap,a[i]);
	}
	soap_dealloc(soap,a);
}
