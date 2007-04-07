#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdsoap2.h>

#include <glite/jp/types.h>
#include "soap_version.h"

#include "jp_H.h"
#include "ws_typemap.h"
#include <glite/security/glite_gscompat.h>
#include "ws_is_typeref.h"

#include "glite/jp/ws_fault.c"


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

void glite_jpis_SoapToAttrOrig(const enum jptype__attrOrig *in, glite_jp_attr_orig_t *out)
{
	assert(out);

	if (!in) {
		*out = GLITE_JP_ATTR_ORIG_ANY;
		return;
	}

        switch ( *in )
        {
        case SYSTEM: *out = GLITE_JP_ATTR_ORIG_SYSTEM; break;
        case USER: *out = GLITE_JP_ATTR_ORIG_USER; break;
        case FILE_: *out = GLITE_JP_ATTR_ORIG_FILE; break;
        default: assert(0); break;
        }
}

static int SoapToQueryRecordVal(
        struct jptype__stringOrBlob	*in,
	int				*binary,
	size_t				*size,
        char				**value)
{
	
        assert(in);
	if (GSOAP_ISSTRING(in)) {
		*binary = 0;
		*size = 0;
		*value = strdup(GSOAP_STRING(in));

		return 0;
	}
	else if (GSOAP_ISBLOB(in)) {
		*binary = 1;
		*size = GSOAP_BLOB(in)->__size;
		memcpy(*value, GSOAP_BLOB(in)->__ptr, GSOAP_BLOB(in)->__size);
		// XXX how to handle id, type, option?

		return 0;
	}
	else 
		// malformed value
		return 1;
}


static int SoapToQueryCond(
        struct jptype__indexQuery	*in,
        glite_jp_query_rec_t		**out)
{
	glite_jp_query_rec_t	*qr;	
	int                     i;
	struct jptype__indexQueryRecord *record;

	assert(in); assert(out);
	qr = calloc(in->__sizerecord + 1, sizeof(*qr));	

	for (i=0; i < in->__sizerecord; i++) {
		record = GLITE_SECURITY_GSOAP_LIST_GET(in->record, i);
		qr[i].attr = strdup(in->attr);
		glite_jpis_SoapToQueryOp(record->op, &(qr[i].op));

		switch (qr[i].op) {
		case GLITE_JP_QUERYOP_EXISTS:
                	break;

		case GLITE_JP_QUERYOP_WITHIN:
			SoapToQueryRecordVal(record->value2, &(qr[i].binary), 
				&(qr[i].size2), &(qr[i].value2));
			// fall through
		default:
			if ( SoapToQueryRecordVal(record->value, &(qr[i].binary), 
					&(qr[i].size), 	&(qr[i].value)) ) {
				*out = NULL;
				return 1;
			}
			break;
		}
		
		glite_jpis_SoapToAttrOrig(in->origin, &(qr[i].origin) );
	}	
	
	*out = qr;

	return 0;
}

/**
 * Translate JP index query conditions from soap to C query_rec 
 *
 * \param OUT array of glite_jp_query_rec_t query records
 */
int glite_jpis_SoapToQueryConds(
	int				size,
        struct jptype__indexQuery	**in,
        glite_jp_query_rec_t		***out)
{
	glite_jp_query_rec_t    **qr;
	int 			i;

	assert(in); assert(out);
        qr = calloc(size+1, sizeof(*qr));

	for (i=0; i<size; i++) {
		if ( SoapToQueryCond(in[i], &(qr[i])) ) {
			*out = NULL;
			return 1;
		}
	}

	*out = qr;
	
	return 0;
}


static int SoapToPrimaryQueryCond(
        struct jptype__primaryQuery	*in,
        glite_jp_query_rec_t		**out)
{
	glite_jp_query_rec_t	*qr;	


	assert(in && in->attr); assert(out);
	*out = qr = calloc(2, sizeof(*qr));	

	qr[0].attr = strdup(in->attr);
	glite_jpis_SoapToQueryOp(in->op, &(qr[0].op));

	switch (qr[0].op) {
	case GLITE_JP_QUERYOP_EXISTS:
		break;

	case GLITE_JP_QUERYOP_WITHIN:
		SoapToQueryRecordVal(in->value2, &(qr[0].binary), 
			&(qr[0].size2), &(qr[0].value2));
		// fall through
	default:
		if ( SoapToQueryRecordVal(in->value, &(qr[0].binary), 
				&(qr[0].size), 	&(qr[0].value)) ) {
			*out = NULL;
			return 1;
		}
		break;
	}
	
	glite_jpis_SoapToAttrOrig(in->origin, &(qr[0].origin) );
	
	return 0;
}



/**
 * Translate JP primary query conditions from soap to C query_rec 
 *
 * \param IN Soap structure
 * \param OUT array of glite_jp_query_rec_t query records
 */
int glite_jpis_SoapToPrimaryQueryConds(
	int				size,
        GLITE_SECURITY_GSOAP_LIST_TYPE(jptype, primaryQuery)	in,
        glite_jp_query_rec_t		***out)
{
	glite_jp_query_rec_t    **qr;
	int 			i;

	assert(in || !size); assert(out);
        qr = calloc(size+1, sizeof(*qr));

	for (i=0; i<size; i++) {
		if ( SoapToPrimaryQueryCond(GLITE_SECURITY_GSOAP_LIST_GET(in, i), &(qr[i])) ) {
			*out = NULL;
			free(qr);
			return 1;
		}
	}

	*out = qr;
	
	return 0;
}
