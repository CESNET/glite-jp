#include <stdlib.h>
#include <string.h>

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

static int attr_cmp(const void *va,const void *vb)
{
	int	c;
	const glite_jp_attr_t	*a = va, *b = vb;

	if (a->type < b->type) return -1;
	if (a->type > b->type) return 1;

	switch (a->type) {
		case GLITE_JP_ATTR_TAG:
		case GLITE_JP_ATTR_GENERIC:
			if (a->namespace && b->namespace && 
				(c = strcmp(a->namespace,b->namespace))) return c;
			return strcmp(a->name,b->name);

		default: return 0;
	}
}

void glite_jp_attr_union(const glite_jp_attr_t *a, const glite_jp_attr_t *b,
	glite_jp_attr_t **out)
{
	int	ac,bc,c,i,j;
	glite_jp_attr_t	*res;

	assert(a); assert(b); assert(out);
	for (ac=0; a[ac].type; ac++);
	for (bc=0; b[bc].type; bc++);
	res = malloc((ac+bc+1) * sizeof *res);
	memcpy(res,a,ac * sizeof *a);
	memcpy(res+ac,b,bc * sizeof *b);
	memset(res+ac+bc,0,sizeof *res);
	c = ac+bc;
	qsort(res,c,sizeof *res,attr_cmp);

	for (i=0; i<c; i++) {
		for (j=i+1; !attr_cmp(res+i,res+j); j++);
		if (j > i+1) memmove(res+i+1,res+j,c-j);
	}

	for (i=0; res[i].type; i++) switch (res[i].type) {
		case GLITE_JP_ATTR_TAG:
		case GLITE_JP_ATTR_GENERIC:
			if (res[i].namespace) res[i].namespace = strdup(res[i].namespace);
			if (res[i].name) res[i].name = strdup(res[i].name);
			break;
		default: break;
	}

	*out = res;
}

void glite_jp_attr_sub(const glite_jp_attr_t *a, const glite_jp_attr_t *b,
	glite_jp_attr_t **out)
{
	abort();
}

void glite_jp_attr_free(glite_jp_attr_t *a,int f)
{
	if (a) {
		switch (a->type) {
			case GLITE_JP_ATTR_TAG:
			case GLITE_JP_ATTR_GENERIC:
				free(a->name);
				free(a->namespace);
				break;
			default:
				break;
		}
		if (f) free(a);
	}
}

void glite_jp_attrset_free(glite_jp_attr_t *a,int f)
{
	int 	i;

	if (a) {
		for (i=0; a[i].type; i++) glite_jp_attr_free(a+i,0);
		if (f) free(a);
	}
}
