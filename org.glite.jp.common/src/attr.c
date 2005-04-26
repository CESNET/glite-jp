#include <stdlib.h>

#include "types.h"

void glite_jp_attrval_free(glite_jp_attrval_t *a,int f)
{
	free(a->attr.name);

	switch (a->attr.type) {
		case GLITE_JP_ATTR_OWNER: 
		case GLITE_JP_ATTR_TAG: 
			free(a->value.s);
			break;
		default: break;
	}

	if (f) free(a);
}
