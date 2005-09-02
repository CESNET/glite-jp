#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdsoap2.h>

#include "glite/jp/types.h"

#include "jpps_H.h"
#include "ws_typemap.h"



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

static void OrigToSoap(struct soap *soap, const glite_jp_attr_orig_t in, enum jptype__attrOrig **out)
{
	enum jptype__attrOrig 	*o = soap_malloc(soap, sizeof(*o));

        switch ( in )
        {
        case GLITE_JP_ATTR_ORIG_ANY: o = NULL; break;
        case GLITE_JP_ATTR_ORIG_SYSTEM: *o = SYSTEM; break;
        case GLITE_JP_ATTR_ORIG_USER: *o = USER; break;
        case GLITE_JP_ATTR_ORIG_FILE: *o = FILE; break;
        default: assert(0); break;
        }
	
	*out = o;
}


static int QueryValToSoap(
        struct soap			*soap,
	int				binary,
	size_t				size,
        char				*in,
        struct jptype__stringOrBlob	**out)
{
	struct jptype__stringOrBlob	*val;

	
        assert(in); assert(out);
	if ( !(val = soap_malloc(soap, sizeof(*val))) ) return SOAP_FAULT;

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
	OrigToSoap(soap, in->origin, &(qr->origin));
	
	switch ( in->op ) {
	case GLITE_JP_QUERYOP_WITHIN:
		if (QueryValToSoap(soap, in->binary, in->size2, in->value2, &qr->value2)) 
			return SOAP_FAULT;
	default:
		if (QueryValToSoap(soap, in->binary, in->size, in->value, &qr->value)) 
                        return SOAP_FAULT;
		break;
	}

	*out = qr;
		
	return SOAP_OK;
}




