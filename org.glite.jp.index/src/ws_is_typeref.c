#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdsoap2.h>

#include <glite/jp/types.h>

#include "jpis_H.h"
#include "ws_typemap.h"
#include "ws_is_typeref.h"


void glite_jpis_SoapToQueryOp(const enum jptype__queryOp in, glite_jp_queryop_t *out)
{
        switch ( in )
        {
        case EQUAL: *out = GLITE_JP_QUERYOP_EQUAL; break;
        case UNEQUAL: *out = GLITE_JP_QUERYOP_UNEQUAL; break;
        case LESS: *out = GLITE_JP_QUERYOP_LESS; break;
        case GREATER: *out = GLITE_JP_QUERYOP_GREATER; break;
        case WITHIN: *out = GLITE_JP_QUERYOP_WITHIN; break;
        case EXISTS: *out = GLITE_JP_QUERYOP_EXISTS; break;
        default: assert(0); break;
        }
}

void glite_jpis_SoapToAttrOrig(struct soap *soap, const enum jptype__attrOrig *in, glite_jp_attr_orig_t *out)
{
	assert(out);

	// XXX: shlouldn't be ANY in WSDL??
	if (!in) {
		*out = GLITE_JP_ATTR_ORIG_ANY;
		return;
	}

        switch ( *in )
        {
//        case NULL: *out = GLITE_JP_ATTR_ORIG_ANY; break;
        case SYSTEM: *out = GLITE_JP_ATTR_ORIG_SYSTEM; break;
        case USER: *out = GLITE_JP_ATTR_ORIG_USER; break;
        case FILE_: *out = GLITE_JP_ATTR_ORIG_FILE; break;
        default: assert(0); break;
        }
}

static int SoapToQueryRecordVal(
        struct soap			*soap,
        struct jptype__stringOrBlob	*in,
	int				*binary,
	size_t				*size,
        char				**value)
{
	
        assert(in);
	if (in->string) {
		*binary = 0;
		*size = 0;
		*value = strdup(in->string);

		return 0;
	}
	else if (in->blob) {
		*binary = 1;
		*size = in->blob->__size;
		memcpy(*value, in->blob->__ptr, in->blob->__size);
		// XXX how to handle id, type, option?

		return 0;
	}
	else 
		// malformed value
		return 1;
}


static int SoapToQueryCond(
        struct soap			*soap,
        struct jptype__indexQuery	*in,
        glite_jp_query_rec_t		**out)
{
	glite_jp_query_rec_t	*qr;	
	int			i;


	assert(in); assert(out);
	qr = calloc(in->__sizerecord, sizeof(*qr));	

	for (i=0; i < in->__sizerecord; i++) {
		qr[i].attr = strdup(in->attr);
		glite_jpis_SoapToQueryOp(in->record[i]->op, &(qr[i].op));

		switch (qr[i].op) {
		case GLITE_JP_QUERYOP_EXISTS:
                	break;

		case GLITE_JP_QUERYOP_WITHIN:
			SoapToQueryRecordVal(soap, in->record[i]->value2, &(qr[i].binary), 
				&(qr[i].size2), &(qr[i].value2));
			// fall through
		default:
			if ( SoapToQueryRecordVal(soap, in->record[i]->value, &(qr[i].binary), 
					&(qr[i].size), 	&(qr[i].value)) ) {
				*out = NULL;
				return 1;
			}
			break;
		}
		
		glite_jpis_SoapToAttrOrig(soap, in->origin, &(qr[i].origin) );
	}	
	
	*out = qr;

	return 0;
}

/**
 * Translate JP query conditions from soap to C query_rec 
 *
 * \param IN Soap structure
 * \param OUT array of glite_jp_query_rec_t query records
 */
int glite_jpis_SoapToQueryConds(
        struct soap			*soap,
	int				size,
        struct jptype__indexQuery	**in,
        glite_jp_query_rec_t		***out)
{
	glite_jp_query_rec_t    **qr;
	int 			i;

	assert(in); assert(out);
        qr = calloc(size+1, sizeof(*qr));

	for (i=0; i<size; i++) {
		if ( SoapToQueryCond(soap, in[i], &(qr[i])) ) {
			*out = NULL;
			return 1;
		}
	}

	*out = qr;
	
	return 0;
}
