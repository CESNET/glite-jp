#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

void glite_jp_attrval_free(glite_jp_attrval_t *a,int f)
{
	free(a->name);
	free(a->value);
	free(a->origin_detail);
	if (f) free(a);
}
