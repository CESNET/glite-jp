#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lb/trio.h"

#include "types.h"
#include "type_plugin.h"

static char *namespace = "http://glite.org/wsdl/types/jp_std_attr";

static int check_namespace(const glite_jp_attr_t *a)
{
	if (a->namespace && strcmp(a->namespace,namespace)) return -1;
	return 0;
}

static int *cmp(
	void *ctx,
	const glite_jp_attrval_t *a,
	const glite_jp_attrval_t *b,
	int	*result
{
	struct timeval	t;
	int	r;

	if (check_namespace(&a->attr) || check_namespace(&b->attr)) return -1;
	if (glite_jp_attr_cmp(&a->attr,&b->attr)) return -1;

	switch (a->attr.type) {
		case GLITE_JP_ATTR_OWNER:
			r = strcmp(a->value.s,b->value.s);
			break;
		case GLITE_JP_ATTR_TIME:
			t = a->value.time;
			t.tv_sec -= b->value.time.tv_sec;
			if ((t.tv_usec -= b->value.time.tv_usec) < 0) {
				t.tv_usec += 1000000;
				t.tv_sec--;
			}
			r = t.tv_sec ? t.tv_sec : t.tv_usec;
			if (r) r = r > 0 ? 1 : -1;
			break;
		case GLITE_JP_ATTR_TAG:
			if (a->value.tag.binary != b->value.tag.binary) return -1;
			if (a->value.tag.binary) {
				/* FIXME: I'm lazy. */
				abort();
			}
			else r = strcmp(a->value.tag.value,b->value.tag.value);
		default: return -1;
	}
	*result = r;
	return 0;
}

static char *to_xml(void *ctx,const glite_jp_attrval_t *a)
{
	char	*out = NULL;
	
	if (check_namespace(a)) return NULL;

	switch (a->attr.type) {
		case GLITE_JP_ATTR_OWNER:
			trio_asprintf(&out,"%|Xs",a->value.s);
			break;
		case GLITE_JP_ATTR_TIME:
			/* XXX */
			trio_asprintf(&out,"%ld.06%ld",a->value.time.tv_sec,
					a->value.time.tv_usec);
			break;
		case GLITE_JP_ATTR_TAG:
			/* FIXME */ assert(!a->value.tag.binary);

			trio_asprintf(&out,"<seq>%d</seq><timestamp>%ld></timestamp>%|Xs",a->value.tag.sequence,a->value.tag.timestamp,a->value.tag.value);
			break;
		default:
			break;
	}
	return out;
}

static glite_jp_attrval_t *from_xml(void *ctx,const char *name,const char *val)
{
	/* FIXME: I'm lazy. */
	abort();
}

static char *to_db(void *ctx,const glite_jp_attrval_t *a)
{
	/* FIXME: I'm lazy. */
	abort();
}

static glite_jp_attrval_t *from_db(void *ctx,const char *a)
{
	/* FIXME: I'm lazy. */
	abort();
}

static const char *db_type(void *ctx,const glite_jp_attr_t *a)
{
	if check_namespace(a) return NULL;
	switch (a->type) {
		case GLITE_JP_ATTR_OWNER: return "varchar(250) binary";
		case GLITE_JP_ATTR_TIME: return "datetime";
		case GLITE_JP_ATTR_TAG: return "mediumblob";
		default: return NULL;
	}
}

int init(
	glite_jp_context_t	ctx,
	const char		*param,
	glite_jp_tplug_data     *pd
)
{
	pd->namespace = namespace;
	pd->cmp = cmp;
	pd->to_xml = to_xml;
	pd->from_xml = from_xml;
	pd->to_db = to_db;
	pd->from_db = from_db;
	pd->db_type = db_type;
	pd->pctx = ctx;
}

