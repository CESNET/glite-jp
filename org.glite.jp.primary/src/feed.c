#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>

#include "glite/jp/types.h"
#include "glite/jp/strmd5.h"
#include "feed.h"
#include "file_plugin.h"
#include "builtin_plugins.h"
#include "is_client.h"

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
	glite_jp_attrval_t	qattr;

	if (strcmp(qry->attr,attr->name)) return 0;

	if (qry->origin != GLITE_JP_ATTR_ORIG_ANY && qry->origin != attr->origin) return 0;

	memset(&qattr,0,sizeof qattr);
	qattr.name = qry->attr;
	qattr.value = qry->value;
	qattr.binary = qry->binary;
	qattr.size = qry->size;
	qattr.origin = qry->origin;

	/* XXX: don't assert */
	assert(glite_jp_attrval_cmp(ctx,attr,&qattr,&cmp) == 0);

	switch (qry->op) {
		case GLITE_JP_QUERYOP_EQUAL: return !cmp;
		case GLITE_JP_QUERYOP_UNEQUAL: return cmp;
		case GLITE_JP_QUERYOP_LESS: return cmp < 0;
		case GLITE_JP_QUERYOP_GREATER: return cmp > 0;

		case GLITE_JP_QUERYOP_WITHIN:
			qattr.value = qry->value2;
			qattr.size = qry->size2;
			/* XXX: assert */
			assert(glite_jp_attrval_cmp(ctx,attr,&qattr,&cmp2) == 0);
			return cmp >= 0 && cmp2 <= 0;
	}
}

/* XXX: limit on query size -- I'm lazy to malloc() */
#define QUERY_MAX	100

static int match_feed(
		glite_jp_context_t ctx,
		const struct jpfeed *feed,
		const char *job,

/* XXX: not checked for correctness,
	assuming single occurence only */
		const glite_jp_attrval_t attrs[] 
)
{
	int	i;
	int	qi[QUERY_MAX];

	glite_jp_attrval_t *newattr = NULL;

	glite_jp_clear_error(ctx);

	if (feed->qry) {
		int	j,complete = 1;

		memset(qi,0,sizeof qi);
		for (i=0; feed->qry[i].attr; i++) {
			int	sat = 0;
			assert(i<QUERY_MAX);
			for (j=0; !sat && attrs[j].name; j++)
				if (!strcmp(attrs[j].name,feed->qry[i].attr)) {
					if (check_qry_item(ctx,feed->qry+i,attrs+j)) { 
						qi[i] = 1;
						sat = 1; /* matched, needn't loop further */
					}
					else return 0; 	/* can't be satisfied either */
				}

			if (!sat) complete = 0;
		}

		/* not all attributes in query are known from input 
		 * we have to retrieve job metadata from the backend
		 * 
		 * XXX: It is not optimal to retrieve it here without sharing
		 * over multiple invocations of match_feed() for the same job.
		 */
		if (!complete) {
			glite_jp_attrval_t	meta[QUERY_MAX+1];
			int	qi2[QUERY_MAX];

			memset(meta,0,sizeof meta);
			j=0;
			for (i=0; feed->qry[i].attr; i++) if (!qi[i]) {
				assert(j<QUERY_MAX);
				meta[j].name = feed->qry[i].attr;
				qi2[j] = i;
				j++;
			}

			if (glite_jppsbe_get_job_metadata(ctx,job,meta)) {
				glite_jp_error_t	err;
				memset(&err,0,sizeof err);
				err.code = EIO;
				err.source = __FUNCTION__;
				err.desc = "complete query";
				return glite_jp_stack_error(ctx,&err);
			}

			for (i=0; meta[i].name; i++)
				if (!check_qry_item(ctx,feed->qry+qi2[i],meta+i))
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
	int	i,j,doit;

	for (;f; f = f->next) {
		doit = 0;

		for (i=0; !doit && f->attrs[i]; i++) 
			for (j=0; !doit && attrs[j].name; j++)
				if (!strcmp(f->attrs[i],attrs[j].name)) doit = 1;

		/* XXX: ignore any errors */
		if (doit) match_feed(ctx,f,job,attrs);
	}

	return glite_jp_clear_error(ctx);
}

static int attr_void_cmp(const void *a, const void *b)
{
	char const * const *ca = (char const * const *) a;
	char const * const *cb = (char const * const *) b;
	return strcmp(*ca,*cb);
}

static void attr_union(char **a, char **b, char ***c)
{
	int	ca = 0,cb = 0,cnt,i,j;
	char	**out;

	if (a) for (ca = 0; a[ca]; ca++);
	if (b) for (cb = 0; b[cb]; cb++);
	out = malloc((ca+cb+1) * sizeof *out);
	if (a) memcpy(out,a,ca * sizeof *out);
	if (b) memcpy(out+ca,b,cb * sizeof *out);
	out[cnt = ca+cb] = NULL;
	qsort(out,cnt,sizeof *out,attr_void_cmp);

	for (i=0; i<cnt; i++) {
		for (j=i; j<cnt && !strcmp(out[i],out[j]); j++);
		if (j < cnt && j > i+1) memmove(out+i+1,out+j,(cnt-j) * sizeof *out);
		cnt -= j-i-1;
	}

	*c = out;
}

int glite_jpps_match_file(
	glite_jp_context_t ctx,
	const char *job,
	const char *class,
	const char *name
)
{
	glite_jpps_fplug_data_t	**pd = NULL;
	int	pi;
	void	*bh = NULL;
	int	ret;
	struct	jpfeed	*f = ctx->feeds;

	int	nvals = 0,j,i;
	char		**attrs = NULL, **attrs2;
	glite_jp_attrval_t	*vals = NULL,*oneval;

	fprintf(stderr,"%s: %s %s %s\n",__FUNCTION__,job,class,name);

	
	switch (glite_jpps_fplug_lookup(ctx,class,&pd)) {
		case ENOENT: return 0;	/* XXX: shall we complain? */
		case 0: break;
		default: return -1;
	}

	for (;f;f=f->next) {
		attr_union(attrs,f->attrs,&attrs2);
		free(attrs);
		attrs = attrs2;
	}

	for (pi=0; pd[pi]; pi++) {
		int	ci;
		for (ci=0; pd[pi]->uris[ci]; ci++) if (!strcmp(pd[pi]->uris[ci],class)) {
			void	*ph;

			if (!bh && (ret = glite_jppsbe_open_file(ctx,job,pd[pi]->classes[ci],name,O_RDONLY,&bh))) {
				free(pd);
				return ret;
			}

			if (pd[pi]->ops.open(pd[pi]->fpctx,bh,class,&ph)) {
				/* XXX: complain more visibly */
				fputs("plugin open failed\n",stderr);
				continue;
			}

			for (i=0; attrs[i]; i++) 
				if (!pd[pi]->ops.attr(pd[pi]->fpctx,ph,attrs[i],&oneval)) {
				/* XXX: ignore error */
					for (j=0; oneval[j].name; j++);
					vals = realloc(vals,(nvals+j+1) * sizeof *vals);
					memcpy(vals+nvals,oneval,(j+1) * sizeof *vals);
					nvals += j;
					free(oneval);
				}

			pd[pi]->ops.close(pd[pi]->fpctx,ph);
		}
	}

	free(attrs);

	for (f = ctx->feeds; f; f=f->next) {
		int 	k;
		glite_jp_attrval_t	* fattr = malloc((nvals+1) * sizeof *fattr);

		j = 0;
		for (i=0; i<nvals; i++) for (k=0; f->attrs[k]; k++)
			if (!strcmp(f->attrs[k],vals[i].name))
				memcpy(fattr+j++,vals+i,sizeof *fattr);

		memset(fattr+j,0,sizeof *fattr);
		glite_jpps_single_feed(ctx,f->destination,job,fattr);
		free(fattr);
	}

	for (i=0; vals[i].name; i++) glite_jp_attrval_free(vals+i,0);
	free(vals);

	if (bh) glite_jppsbe_close_file(ctx,bh);
	free(pd);

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

static struct jpfeed *make_jpfeed(
	const char *destination,
	char const *const *attrs,
	const glite_jp_query_rec_t *qry,
	char *id,
	time_t expires)
{
	int	i;
	struct jpfeed	*f = calloc(1,sizeof *f);

	f->id = id ? strdup(id) : NULL;
	f->destination = strdup(destination);
	f->expires = expires;
	for (i=0; attrs[i]; i++) {
		f->attrs = realloc(f->attrs,(i+2) * sizeof *f->attrs);
		f->attrs[i] = strdup(attrs[i]);
		f->attrs[i+1] = NULL;
	}
	for (i=0; qry[i].attr; i++) {
		f->qry = realloc(f->qry,(i+2) * sizeof *f->qry);
		glite_jp_queryrec_copy(f->qry+i,qry+i);
		memset(f->qry+i+1,0,sizeof *f->qry);
	}

	return f;
}

void jpfeed_free(struct jpfeed *f)
{
	/* TODO */
	abort();
}

static int feed_query_callback(
		glite_jp_context_t ctx,
		const char *job,
		const glite_jp_attrval_t attr[],
		void *arg)
{
	struct jpfeed	*f = arg;
	glite_jp_attrval_t	*attrs = NULL;

	glite_jpps_get_attrs(ctx,job,f->other_attr,f->nother_attr,&attrs);
}

static int run_feed_deferred(glite_jp_context_t ctx,void *feed)
{
	struct jpfeed	*f = feed;
	int	i,m,o,cnt,ret = 0;

	glite_jp_clear_error(ctx);
/* count "meta" attributes */
	cnt = 0;
	for (i=0; f->attrs[i]; f++)
		if (glite_jppsbe_is_metadata(ctx,f->attrs[i])) cnt++;

	f->meta_attr = cnt ? malloc(cnt * sizeof *f->meta_attr) : NULL;
	f->nmeta_attr = cnt;

	f->other_attr = i-cnt ? malloc((i-cnt) * sizeof *f->other_attr) : NULL;

/* sort attributes to "meta" and others */
	m = o = 0;
	for (i=0; f->attrs[i]; i++)
		if (glite_jppsbe_is_metadata(ctx,f->attrs[i])) f->meta_attr[m++] = f->attrs[i];
		else f->other_attr[o++] = f->attrs[i];

	if (f->meta_attr) f->meta_attr[m] = NULL;
	if (f->other_attr) f->other_attr[o] = NULL;

/* the same for query records */
	cnt = 0;
	for (i=0; f->qry[i].attr; i++)
		if (glite_jppsbe_is_metadata(ctx,f->qry[i].attr)) cnt++;

	f->meta_qry = malloc(cnt * sizeof *f->meta_qry);
	f->nmeta_qry = cnt;
	f->other_qry = malloc((i-cnt) * sizeof *f->other_qry);
	f->nother_qry = i-cnt;

	m = o = 0;
	for (i=0; f->qry[i].attr; i++) 
		if (glite_jppsbe_is_metadata(ctx,f->qry[i].attr)) memcpy(f->meta_qry+m++,f->qry+i,sizeof *f->meta_qry);
		else memcpy(f->other_qry+o++,f->qry+i,sizeof *f->other_qry);

	memset(f->meta_qry+m,0,sizeof *f->meta_qry);
	memset(f->other_qry+m,0,sizeof *f->other_qry);

	free(f->attrs); free(f->qry);
	f->attrs = NULL;
	f->qry = NULL;

/* extract other_qry items that are not present in other_attr */
	f->int_other_attr = o;
	for (i=0; i<f->nother_qry; i++) {
		int	j;
		for (j=0; j<f->int_other_attr && strcmp(f->other_attr[j],f->other_qry[i].attr); j++);
		if (j == f->int_other_attr) {
			f->other_attr = realloc(f->other_attr,(o+1) * sizeof *f->other_attr);
			f->other_attr[o++] = strdup(f->other_qry[i].attr);
		}
	}
	if (f->other_attr) f->other_attr[o] = NULL;
	f->nother_attr = o;

	ret = glite_jppsbe_query(ctx,f->meta_qry,f->meta_attr,f,feed_query_callback);

cleanup:

	jpfeed_free(f);
	return ret;
}

int glite_jpps_run_feed(
	glite_jp_context_t ctx,
	const char *destination,
	char const * const *attrs,
	const glite_jp_query_rec_t *qry,
	char **feed_id)
{
	struct jpfeed	*f;

	fprintf(stderr,"%s: \n",__FUNCTION__);
	if (!*feed_id) *feed_id = generate_feedid();

       	f = make_jpfeed(destination,attrs,qry,*feed_id,(time_t) 0);
	glite_jp_add_deferred(ctx,run_feed_deferred,f);

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
	char const *const *attrs,
	const glite_jp_query_rec_t *qry,
	char **feed_id,
	time_t *expires)
{
	struct jpfeed	*f;

	if (!*feed_id) *feed_id = generate_feedid();
	time(expires); *expires += FEED_TTL;

       	f = make_jpfeed(destination,attrs,qry,*feed_id,*expires);
	glite_jp_add_deferred(ctx,register_feed_deferred,f);

	return 0;
}

