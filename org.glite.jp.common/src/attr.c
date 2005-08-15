#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "types.h"
#include "attr.h"
#include "type_plugin.h"

void glite_jp_attrval_free(glite_jp_attrval_t *a,int f)
{
	free(a->name);
	free(a->value);
	free(a->origin_detail);
	if (f) free(a);
}

static glite_jp_tplug_data_t *get_plugin(glite_jp_context_t ctx,const glite_jp_attrval_t *a)
{
	void	**cp = ctx->type_plugins;
	char	*colon,*ns;

	assert(cp);
	glite_jp_clear_error(ctx);
	ns = strdup(a->name);
	colon = strrchr(ns,':');
	if (colon) *colon = 0; else *ns = 0;

	while (*cp) {
		glite_jp_tplug_data_t	*p = *cp;
		if (!strcmp(ns,p->namespace)) {
			free(ns);
			return p;
		}
	}
	free(ns);
	return NULL;
}

#define check_ap(ap,attr,eret) \
	if (!(ap)) {	\
		err.code = ENOENT;	\
		snprintf(ebuf,sizeof ebuf - 1,	\
			"Can't find type plugin for %s",(attr)->name);	\
		ebuf[sizeof ebuf - 1] = 0;	\
		err.desc = ebuf;	\
		glite_jp_stack_error(ctx,&err);	\
		return eret; \
	}


int glite_jp_attrval_cmp(glite_jp_context_t ctx,const glite_jp_attrval_t *a,const glite_jp_attrval_t *b,int *result)
{
	glite_jp_tplug_data_t	*ap = get_plugin(ctx,a);
	glite_jp_error_t	err;
	char	ebuf[BUFSIZ];

	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	glite_jp_clear_error(ctx);

	if (strcmp(a->name,b->name)) {
		err.code = EINVAL;
		err.desc = "Can't compare different attributes";
		return glite_jp_stack_error(ctx,&err);
	}

	check_ap(ap,a,err.code);

	return ap->cmp(ap->pctx,a,b,result);
}

char *glite_jp_attrval_to_db_full(glite_jp_context_t ctx,const glite_jp_attrval_t *attr)
{
	glite_jp_tplug_data_t	*ap = get_plugin(ctx,attr);
	glite_jp_error_t	err;
	char	ebuf[BUFSIZ];
	int	result;

	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	glite_jp_clear_error(ctx);

	check_ap(ap,attr,NULL);
	return ap->to_db_full(ap->pctx,attr);
}

char *glite_jp_attrval_to_db_index(glite_jp_context_t ctx,const glite_jp_attrval_t *attr,int len)
{
}


int *glite_jp_attrval_from_db(glite_jp_context_t ctx,const char *str,glite_jp_attrval_t *attr)
{
}

const char *glite_jp_attrval_db_type_full(glite_jp_context_t ctx,const char *attr)
{
}

const char *glite_jp_attrval_db_type_index(glite_jp_context_t ctx,const char *attr,int len)
{
}



