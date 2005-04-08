#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "glite/jp/types.h"
#include "glite/jp/strmd5.h"
#include "feed.h"


/* 
 * seconds before feed expires: should be 
 * XXX: should be configurable, default for real deployment sort of 1 hour
 */
#define FEED_TTL	120

static int check_qry_item(
		glite_jp_context_t ctx,
		const glite_jp_query_rec_t    *qry,
		const glite_jp_attrval_t *attr
)
{
	int	cmp,cmp2;
	long	scmp,ucmp;

	switch (qry->attr.type) {
		case GLITE_JP_ATTR_OWNER:
		case GLITE_JP_ATTR_TAG:
			cmp = strcmp(attr->value.s,qry->value.s);
			break;
		case GLITE_JP_ATTR_TIME:
			scmp = (ucmp = attr->value.time.tv_usec - qry->value.time.tv_usec) > 0 ? 0 : -1;
			ucmp -= 1000000 * scmp;
			scmp += attr->value.time.tv_sec - qry->value.time.tv_sec;
			cmp = scmp ? scmp : ucmp;
			break;
	}
	switch (qry->op) {
		case GLITE_JP_QUERYOP_EQUAL: return !cmp;
		case GLITE_JP_QUERYOP_UNEQUAL: return cmp;
		case GLITE_JP_QUERYOP_LESS: return cmp < 0;
		case GLITE_JP_QUERYOP_GREATER: return cmp > 0;

		case GLITE_JP_QUERYOP_WITHIN:
			switch (qry->attr.type) {
			case GLITE_JP_ATTR_OWNER:
			case GLITE_JP_ATTR_TAG:
				cmp2 = strcmp(attr->value.s,qry->value2.s);
				break;
			case GLITE_JP_ATTR_TIME:
				scmp = (ucmp = attr->value.time.tv_usec - qry->value2.time.tv_usec) > 0 ? 0 : -1;
				ucmp -= 1000000 * scmp;
				scmp += attr->value.time.tv_sec - qry->value2.time.tv_sec;
				cmp2 = scmp ? scmp : ucmp;
				break;
			}
			return cmp >= 0 && cmp2 <= 0;
	}
}

/* XXX: limit on query size -- I'm lazy to malloc() */
#define QUERY_MAX	100

static int match_feed(
		glite_jp_context_t ctx,
		const struct jpfeed *feed,
		const char *job,
		const glite_jp_attrval_t attrs[] /* XXX: not checked for correctness */
)
{
	int	i;
	int	attri[GLITE_JP_ATTR__LAST];
	int	qi[QUERY_MAX];

	glite_jp_attrval_t *newattr = NULL;

	glite_jp_clear_error(ctx);

	for (i=0; i<GLITE_JP_ATTR__LAST; i++) attri[i] = -1;
	for (i=0; attrs[i].attr.type; i++) attri[attrs[i].attr.type] = i;

	if (feed->qry) {
		int	j,complete = 1;

		memset(qi,0,sizeof qi);
		for (i=0; feed->qry[i].attr.type; i++) {
			assert(i<QUERY_MAX);
			if ((j=attri[feed->qry[i].attr.type]) >=0) {
				if (check_qry_item(ctx,feed->qry+i,attrs+j))
					qi[i] = 1; /* matched */
				else return 0;  /* can't be satisfied */
			}
			else complete = 0;
		}

		/* not all attributes in query are known from input 
		 * we have to retrieve job metadata from the backend
		 */
		if (!complete) {
			glite_jp_attrval_t	meta[GLITE_JP_ATTR__LAST+1];
			int	qai[GLITE_JP_ATTR__LAST];

			memset(meta,0,sizeof meta);
			j=0;
			for (i=0; feed->qry[i].attr.type; i++) if (!qi[i]) {
				meta[j].attr.type = feed->qry[i].attr.type;
				meta[j].attr.name = feed->qry[i].attr.name;
				qai[feed->qry[i].attr.type] = i;
				j++;
			}

			if (glite_jppsbe_get_job_metadata(ctx,job,meta)) {
				glite_jp_error_t	err;
				err.code = EIO;
				err.source = __FUNCTION__;
				err.desc = "complete query";
				return glite_jp_stack_error(ctx,&err);
			}

			for (i=0; j=meta[i].attr.type; i++)
				if (!check_qry_item(ctx,feed->qry+qai[j],meta+i))
					return 0;
		}
	}

	/* matched completely */
	return glite_jpps_single_feed(ctx,feed->destination,job,attrs);
	return 0;
}

int glite_jpps_match_attr(
		glite_jp_context_t ctx,
		const char *job,
		const glite_jp_attrval_t attrs[]
)
{
	struct jpfeed	*f = (struct jpfeed *) ctx->feeds;
	int	i,j;
	int 	attri[GLITE_JP_ATTR__LAST];

	glite_jp_clear_error(ctx);

	for (i=0; i<GLITE_JP_ATTR__LAST; i++) attri[i] = -1;
	for (i=0; attrs[i].attr.type; i++) {
		if (attrs[i].attr.type >= GLITE_JP_ATTR__LAST ||
				attrs[i].attr.type <= 0)
		{
			glite_jp_error_t	err;
			err.code = EINVAL;
			err.source = __FUNCTION__;
			err.desc = "unknown attribute";
			return glite_jp_stack_error(ctx,&err);
		}
		if (attri[attrs[i].attr.type] >= 0) {
			glite_jp_error_t	err;
			err.code = EINVAL;
			err.source = __FUNCTION__;
			err.desc = "double attribute change";
			return glite_jp_stack_error(ctx,&err);
		}

		attri[attrs[i].attr.type] = i;
	}

	for (;f; f = f->next) {
		for (i=0; f->attrs[i].type && attri[f->attrs[i].type] == -1; i++);
		/* XXX: ignore any errors */
		if (f->attrs[i].type) match_feed(ctx,f,job,attrs);
	}

	return glite_jp_clear_error(ctx);
}

int glite_jpps_match_file(
	glite_jp_context_t ctx,
	const char *job,
	const char *class,
	const char *name
)
{
	fprintf(stderr,"%s: \n",__FUNCTION__);
	return 0;
}

int glite_jpps_match_tag(
	glite_jp_context_t ctx,
	const char *job,
	const glite_jp_tagval_t *tag
)
{
	fprintf(stderr,"%s: \n",__FUNCTION__);
	return 0;
}

static char *generate_feedid(void)
{
	char	hname[200],buf[1000];

	gethostname(hname,sizeof hname);
	snprintf(buf,sizeof buf,"%s%d%ld",hname,getpid(),lrand48());
	buf[sizeof buf-1] = 0;
	return str2md5base64(buf);
}


int glite_jpps_run_feed(
	glite_jp_context_t ctx,
	const char *destination,
	const glite_jp_attr_t *attrs,
	const glite_jp_query_rec_t *qry,
	char **feed_id)
{
	fprintf(stderr,"%s: \n",__FUNCTION__);
	return 0;
}

static int register_feed_deferred(glite_jp_context_t ctx,void *feed)
{
	struct jpfeed	*f = feed;

	f->next = ctx->feeds;
	ctx->feeds = f;
	return 0;
}

/* FIXME:
 * - volatile implementation: should store the registrations in a file
 *   and recover after restart
 * - should communicate the data among all server slaves
 */
int glite_jpps_register_feed(
	glite_jp_context_t ctx,
	const char *destination,
	const glite_jp_attr_t *attrs,
	const glite_jp_query_rec_t *qry,
	char **feed_id,
	time_t *expires)
{
	int	i;
	struct jpfeed	*f = calloc(1,sizeof *f);

	if (!*feed_id) *feed_id = generate_feedid();
	time(expires); *expires += FEED_TTL;

	f->id = strdup(*feed_id);
	f->destination = strdup(destination);
	f->expires = *expires;
	for (i=0; attrs[i].type; i++) {
		f->attrs = realloc(f->attrs,(i+2) * sizeof *f->attrs);
		glite_jp_attr_copy(f->attrs+i,attrs+i);
		memset(f->attrs+i+1,0,sizeof *f->attrs);
	}
	for (i=0; qry[i].attr.type; i++) {
		f->qry = realloc(f->qry,(i+2) * sizeof *f->qry);
		glite_jp_queryrec_copy(f->qry+i,qry+i);
		memset(f->qry+i+1,0,sizeof *f->qry);
	}

	glite_jp_add_deferred(ctx,register_feed_deferred,f);

	return 0;
}

