#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdsoap2.h>

#include <glite/jp/types.h>

#include "jpps_H.h"
#include "ws_typemap.h"
#include "ws_ps_typeref.h"


static void QueryOpToSoap(const glite_jp_queryop_t in, enum jptype__queryOp *out)
{
        switch ( in )
        {
        case GLITE_JP_QUERYOP_EQUAL: *out = EQUAL; break;
        case GLITE_JP_QUERYOP_UNEQUAL: *out = UNEQUAL; break;
        case GLITE_JP_QUERYOP_LESS: *out = LESS; break;
        case GLITE_JP_QUERYOP_GREATER: *out = GREATER; break;
        case GLITE_JP_QUERYOP_WITHIN: *out = WITHIN; break;
        case GLITE_JP_QUERYOP_EXISTS: *out = EXISTS; break;
        default: assert(0); break;
        }
}

void glite_jpis_AttrOrigToSoap(struct soap *soap, const glite_jp_attr_orig_t in, enum jptype__attrOrig **out)
{
	enum jptype__attrOrig 	*o = soap_malloc(soap, sizeof(*o));

        switch ( in )
        {
        case GLITE_JP_ATTR_ORIG_ANY: o = NULL; break;
        case GLITE_JP_ATTR_ORIG_SYSTEM: *o = SYSTEM; break;
        case GLITE_JP_ATTR_ORIG_USER: *o = USER; break;
        case GLITE_JP_ATTR_ORIG_FILE: *o = FILE_; break;
        default: assert(0); break;
        }
	
	*out = o;
}

static int QueryRecordValToSoap(
        struct soap			*soap,
	int				binary,
	size_t				size,
        char				*in,
        struct jptype__stringOrBlob	**out)
{
	struct jptype__stringOrBlob	*val;

	
        assert(out);
	if ( !(val = soap_malloc(soap, sizeof(*val))) ) return SOAP_FAULT;
	memset(val, 0, sizeof(*val) );

        if (binary) {
		val->string = NULL;
		if ( !(val->blob = soap_malloc(soap, sizeof(*val->blob))) ) return SOAP_FAULT;
		val->blob->__size = size;
		if ( !(val->blob->__ptr = soap_malloc(soap, val->blob->__size)) ) return SOAP_FAULT;
		memcpy(val->blob->__ptr, in, val->blob->__size);
		// XXX how to handle id, type, option?
	}
	else {
		val->blob = NULL;
		if ( !(val->string = soap_strdup(soap, in)) )  return SOAP_FAULT;
	}

	*out = val;

	return SOAP_OK;
}

/**
 * Translate JP query condition from C query_rec to Soap
 *
 * \param IN in glite_jp_query_rec_t query record
 * \param OUT Soap structure
 */
int glite_jpis_QueryCondToSoap(
	struct soap                     *soap,
	glite_jp_query_rec_t 		*in, 
	struct jptype__primaryQuery	**out)
{
	struct jptype__primaryQuery	*qr;

	assert(in); assert(out);
	if ( !(qr = soap_malloc(soap, sizeof(*qr))) ) return SOAP_FAULT;
	memset(qr, 0, sizeof(*qr));

	if ( !(qr->attr = soap_strdup(soap, in->attr)) ) return SOAP_FAULT;
	QueryOpToSoap(in->op, &(qr->op));
	glite_jpis_AttrOrigToSoap(soap, in->origin, &(qr->origin));
	
	switch ( in->op ) {
	case GLITE_JP_QUERYOP_WITHIN:
		if (QueryRecordValToSoap(soap, in->binary, in->size2, in->value2, &qr->value2)) 
			return SOAP_FAULT;
	default:
		if (QueryRecordValToSoap(soap, in->binary, in->size, in->value, &qr->value)) 
                        return SOAP_FAULT;
		break;
	}

	*out = qr;
		
	return SOAP_OK;
}

static void SoapToAttrOrig(glite_jp_attr_orig_t *out, const enum jptype__attrOrig in)
{
        switch ( in )
        {
	case jptype__attrOrig__SYSTEM: *out = GLITE_JP_ATTR_ORIG_SYSTEM; break;
	case jptype__attrOrig__USER: *out = GLITE_JP_ATTR_ORIG_USER; break;
	case jptype__attrOrig__FILE_: *out = GLITE_JP_ATTR_ORIG_FILE; break;
	default: assert(0); break;
        }
}

void glite_jpis_SoapToAttrVal(glite_jp_attrval_t *av, const struct jptype__attrValue *attr) {
	memset(av, 0, sizeof(*av));
	av->name = attr->name;
	av->binary = attr->value->blob ? 1 : 0;
	assert(av->binary || attr->value->string);
	if (av->binary) {
		av->value = attr->value->blob->__ptr;
		av->size =attr->value->blob->__size ;
	} else {
		av->size = -1;
	}
	SoapToAttrOrig(&av->origin, attr->origin);
	av->origin_detail = attr->originDetail;
	av->timestamp = attr->timestamp;
}