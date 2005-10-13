#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "strmd5.h"
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

void glite_jp_attrval_copy(glite_jp_attrval_t *dst,const glite_jp_attrval_t *src)
{
	dst->name = strdup(src->name);
	dst->origin = src->origin;
	dst->size = src->size;
	dst->timestamp = src->timestamp;
	dst->origin_detail = src->origin_detail ? 
		strdup(src->origin_detail) : NULL;
	if (dst->binary = src->binary) {
		dst->value = malloc(src->size);
		memcpy(dst->value,src->value,src->size);
	}
	else dst->value = strdup(src->value);
}


#define min(x,y) ((x) > (y) ? (y) : (x))

static int fb_cmp(void *ctx,const glite_jp_attrval_t *a,const glite_jp_attrval_t *b,int *result)
{
	if (a->binary != b->binary) return EINVAL;
	if (a->binary) {
		*result = memcmp(a->value,b->value,min(a->size,b->size));
		if (!*result && a->size != b->size) 
			*result = a->size > b->size ? 1 : -1;
	}
	else *result = strcmp(a->value,b->value);
	return 0;
}

static char * fb_to_db_full(void *ctx,const glite_jp_attrval_t *attr)
{
	char	*db;
	if (attr->binary) {
		int	osize = attr->size * 4/3 + 6;
		db = malloc(osize);
		db[0] = 'B'; db[1] = ':';
		osize = base64_encode(attr->value,attr->size,db+2,osize-3);
		assert(osize >= 0);
		db[osize] = 0;
	}
	else {
		db = malloc(strlen(attr->value)+3);
		db[0] = 'S'; db[1] = ':';
		strcpy(db+2,attr->value);
	}
	return db;
}

static char * fb_to_db_index(void *ctx,const glite_jp_attrval_t *attr,int len)
{
	char	*db = fb_to_db_full(ctx,attr);
	if (len < strlen(db)) db[len] = 0;
	return db;
}

int fb_from_db(void *ctx,const char *str,glite_jp_attrval_t *attr)
{
	int	osize;
	switch (str[0]) {
		case 'B':
			attr->value = malloc(osize = strlen(str) * 3/4 + 4);
			attr->size = base64_decode(str,attr->value+2,osize);
			assert(attr->size >= 0);
			attr->binary = 1;
			break;
		case 'S':
			attr->value = strdup(attr->value + 2);
			attr->size = 0;
			attr->binary = 0;
			break;
		default: return EINVAL;
	}
	return 0;
}

static const char * fb_type_full(void *ctx,const char *attr)
{
	return "mediumblob";
}

static const char * fb_type_index(void *ctx,const char *attr,int len)
{
	static char tbuf[100];
	sprintf(tbuf,"varchar(%d)",len);
	return tbuf;
}



static glite_jp_tplug_data_t fallback_plugin = {
	"",
	NULL,
	fb_cmp,
	fb_to_db_full,
	fb_to_db_index,
	fb_from_db,
	fb_type_full,
	fb_type_index,
};

static glite_jp_tplug_data_t *get_plugin(glite_jp_context_t ctx,const char *aname)
{
	void	**cp = ctx->type_plugins;
	char	*colon,*ns;

	if (!cp) return &fallback_plugin;
	glite_jp_clear_error(ctx);
	ns = strdup(aname);
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
	return &fallback_plugin;	/* XXX: is it always desirable? */
}

int glite_jp_attrval_cmp(glite_jp_context_t ctx,const glite_jp_attrval_t *a,const glite_jp_attrval_t *b,int *result)
{
	glite_jp_tplug_data_t	*ap = get_plugin(ctx,a->name);
	glite_jp_error_t	err;

	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	glite_jp_clear_error(ctx);

	if (strcmp(a->name,b->name)) {
		err.code = EINVAL;
		err.desc = "Can't compare different attributes";
		return glite_jp_stack_error(ctx,&err);
	}

	return ap->cmp(ap->pctx,a,b,result);
}

char *glite_jp_attrval_to_db_full(glite_jp_context_t ctx,const glite_jp_attrval_t *attr)
{
	glite_jp_tplug_data_t	*ap = get_plugin(ctx,attr->name);

	glite_jp_clear_error(ctx);
	return ap->to_db_full(ap->pctx,attr);
}

char *glite_jp_attrval_to_db_index(glite_jp_context_t ctx,const glite_jp_attrval_t *attr,int len)
{
	glite_jp_tplug_data_t	*ap = get_plugin(ctx,attr->name);

	glite_jp_clear_error(ctx);
	return ap->to_db_index(ap->pctx,attr,len);
}


int glite_jp_attrval_from_db(glite_jp_context_t ctx,const char *str,glite_jp_attrval_t *attr)
{
	glite_jp_tplug_data_t	*ap = get_plugin(ctx,attr->name);

	glite_jp_clear_error(ctx);
	return ap->from_db(ap->pctx,str,attr);
}

const char *glite_jp_attrval_db_type_full(glite_jp_context_t ctx,const char *attr)
{
	glite_jp_tplug_data_t	*ap = get_plugin(ctx,attr);

	glite_jp_clear_error(ctx);
	return ap->db_type_full(ap->pctx,attr);
}

const char *glite_jp_attrval_db_type_index(glite_jp_context_t ctx,const char *attr,int len)
{
	glite_jp_tplug_data_t	*ap = get_plugin(ctx,attr);

	glite_jp_clear_error(ctx);
	return ap->db_type_index(ap->pctx,attr,len);
}

/* XXX: UNIX time, should be ISO blahblah */
time_t glite_jp_attr2time(const char *a)
{
	long	t;

	sscanf(a,"%ld",&t);
	return t;
}

/* XXX: UNIX time, should be ISO blahblah */
char * glite_jp_time2attr(time_t t)
{
	char	*r;

	trio_asprintf(&r,"%ld",(long) t);
	return r;
}

