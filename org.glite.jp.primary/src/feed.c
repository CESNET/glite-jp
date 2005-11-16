#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/strmd5.h"
#include "glite/jp/known_attr.h"
#include "feed.h"
#include "file_plugin.h"
#include "builtin_plugins.h"
#include "is_client.h"
#include "backend.h"

extern pid_t	master;

/* 
 * seconds before feed expires: should be 
 * XXX: should be configurable, default for real deployment sort of 1 hour
 */
#define FEED_TTL	3600

/* XXX: configurable */
#define BATCH_FEED_SIZE	200

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

/* retrieve all attributes for a feed */
int full_feed(
	glite_jp_context_t ctx,
	const struct jpfeed *feed,
	const char *job,
	glite_jp_attrval_t **attrs)
{
	int	i,ret;
	char	**ma;

	for (i=0; feed->attrs[i]; i++);
	ma = malloc((i+2) * sizeof *ma);
	ma[0] = GLITE_JP_ATTR_OWNER;
	memcpy(ma+1,feed->attrs,(i+1) * sizeof *ma);
	ret = glite_jpps_get_attrs(ctx,job,ma,i+1,attrs);
	free(ma);
	return ret;
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
	int	i,fed,ret = 0;
	int	qi[QUERY_MAX];
	char	*owner = NULL;
	glite_jp_attrval_t	meta[QUERY_MAX+1];
	glite_jp_attrval_t *newattr = NULL;

	glite_jp_clear_error(ctx);
	memset(meta,0,sizeof meta);

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
			int	qi2[QUERY_MAX];

			memset(meta,0,sizeof meta);
			j=0;
			for (i=0; feed->qry[i].attr; i++) if (!qi[i]) {
				assert(j<QUERY_MAX);
				meta[j].name = strdup(feed->qry[i].attr);
				qi2[j] = i;
				j++;
			}

			if (glite_jppsbe_get_job_metadata(ctx,job,meta)) {
				glite_jp_error_t	err;

				memset(&err,0,sizeof err);
				err.code = EIO;
				err.source = __FUNCTION__;
				err.desc = "complete query";
				ret = glite_jp_stack_error(ctx,&err);
				goto cleanup;
			}

			for (i=0; meta[i].name; i++) {
				if (!check_qry_item(ctx,feed->qry+qi2[i],meta+i)) {
					ret = 0;
					goto cleanup;
				}
				if (!strcmp(meta[i].name,GLITE_JP_ATTR_OWNER)) owner = meta[i].value;
			}
		}
	}

	/* matched completely */
	glite_jppsbe_check_fed(ctx,feed->id,job,&fed);
	if (!fed) {
		glite_jp_attrval_t	*a;
		full_feed(ctx,feed,job,&a);
		for (i=0; a[i].name && strcmp(a[i].name,GLITE_JP_ATTR_OWNER); i++);
		owner = a[i].value;

		glite_jpps_single_feed(ctx,feed->id,0,feed->destination,job,owner,a);
		for (i=0; a[i].name; i++) glite_jp_attrval_free(a+i,0);
		free(a);
	}
	else {
		if (!owner) {
			glite_jppsbe_get_job_metadata(ctx,job,meta);
			for (i=0; meta[i].name && strcmp(meta[i].name,GLITE_JP_ATTR_OWNER); i++);
		}
		glite_jpps_single_feed(ctx,feed->id,0,feed->destination,job,owner,attrs);
	}

cleanup:
	for (i=0; meta[i].name; i++) glite_jp_attrval_free(meta+i,0);
	return ret;
}

/* TODO: overit, ze do dalsich atributu se leze az kdyz matchuji metadata
 * kdyby ne, stejne se to nepovede ;
 * totez pro match_file */

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
	glite_jp_attrval_t	meta[QUERY_MAX+1];

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

	if (bh) glite_jppsbe_close_file(ctx,bh);
	free(pd);

	for (f = ctx->feeds; f; f=f->next) {
		int 	k,fed;
		glite_jp_attrval_t	* fattr;

		glite_jppsbe_check_fed(ctx,f->id,job,&fed);
		if (!fed) full_feed(ctx,f,job,&fattr);
		else {
			fattr = malloc((nvals+1) * sizeof *fattr);

			j = 0;
			for (i=0; i<nvals; i++) for (k=0; f->attrs[k]; k++)
				if (!strcmp(f->attrs[k],vals[i].name))
					memcpy(fattr+j++,vals+i,sizeof *fattr);

			memset(fattr+j,0,sizeof *fattr);

		}
		glite_jppsbe_get_job_metadata(ctx,job,meta);
		for (i=0; meta[i].name && strcmp(meta[i].name,GLITE_JP_ATTR_OWNER); i++);
		glite_jpps_single_feed(ctx,f->id,0,f->destination,job,meta[i].value,fattr);
		for (i=0; meta[i].name; i++) glite_jp_attrval_free(meta+i,0);
		if (!fed) for (i=0; fattr[i].name; i++) glite_jp_attrval_free(fattr+i,0);
		free(fattr);
	}

	for (i=0; vals[i].name; i++) glite_jp_attrval_free(vals+i,0);
	free(vals);

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
	int	i;

	assert(f->njobs == 0); /* XXX: we shouldn't do this */

	free(f->id);
	free(f->destination);
	if (f->attrs) {
		for (i=0; f->attrs[i]; i++) free(f->attrs[i]);
		free(f->attrs);
	}
	for (i=0; i<f->nmeta_attr; i++) free(f->meta_attr[i]);
	free(f->meta_attr);
	for (i=0; i<f->nother_attr; i++) free(f->other_attr[i]);
	free(f->other_attr);

	if (f->qry) {
		for (i=0; f->qry[i].attr; i++) glite_jp_free_query_rec(f->qry+i);
		free(f->qry);
	}

	for (i=0; i<f->nmeta_qry; i++) glite_jp_free_query_rec(f->meta_qry+i);
	free(f->meta_qry);
	for (i=0; i<f->nother_qry; i++) glite_jp_free_query_rec(f->other_qry+i);
	free(f->other_qry);

	/* XXX: no next */

	free(f);
}

static void drop_jobs(struct jpfeed *f)
{
	int	i,j;
	for (i=0; i<f->njobs; i++) {
		for (j=0; f->job_attrs[i][j].name; j++)
			glite_jp_attrval_free(&f->job_attrs[i][j],0);
		free(f->job_attrs[i]);
		free(f->jobs[i]);
		free(f->owners[i]);
	}
	free(f->job_attrs);
	free(f->jobs);
	free(f->owners);
	f->job_attrs = NULL;
	f->jobs = NULL;
	f->njobs = 0;
}

static int drain_feed(glite_jp_context_t ctx, struct jpfeed *f,int done)
{
	int	ret = 0;
	glite_jp_clear_error(ctx);
	if (f->njobs) {
		ret = glite_jpps_multi_feed(ctx,f->id,done,f->njobs,f->destination,f->jobs,f->owners,f->job_attrs);
		drop_jobs(f);
	}
	return ret;
}

static int feed_query_callback(
		glite_jp_context_t ctx,
		const char *job,
		const glite_jp_attrval_t meta[],
		void *arg)
{
	int	i,j,nout = 0,ec;
	glite_jp_error_t	err;
	struct jpfeed	*f = arg;
	glite_jp_attrval_t	*other = NULL,*out = NULL;

	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	glite_jp_clear_error(ctx);

/* retrieve other attributes */
	ec = glite_jpps_get_attrs(ctx,job,f->other_attr,f->nother_attr,&other);
	switch (ec) {
		case 0: break;
		case ENOENT: glite_jp_clear_error(ctx); break;
		default:
			err.code = EIO;
			err.desc = "retrieve job attributes";
			glite_jp_stack_error(ctx,&err);
			goto cleanup;
	}

/* no attributes known -- can't match */
	if (!other) goto cleanup;

/* filter on non-meta query items */
	for (i=0; i<f->nother_qry; i++) {
		for (j=0; other[j].name; j++) 
			if (check_qry_item(ctx,f->other_qry+i,other+j)) break;
		if (!other[j].name) goto cleanup; /* no match is not an error */
	}

/* extract attributes to be fed, stack the job for a batch feed */
	for (i=0; meta && meta[i].name; i++)
		for (j=0; j<f->nmeta_attr; j++) 
			if (!strcmp(meta[i].name,f->meta_attr[j])) {
				out = realloc(out,(nout+2) * sizeof *out);
				glite_jp_attrval_copy(out+nout,meta+i);
				nout++;
			}

	for (i=0; other[i].name; i++) 
		for (j=0; j<f->int_other_attr; j++)
			if (!strcmp(other[i].name,f->other_attr[j])) {
				out = realloc(out,(nout+2) * sizeof *out);
				glite_jp_attrval_copy(out+nout,other+i);
				nout++;
			}

	if (nout) {
		int	oi;

		memset(out+nout,0,sizeof *out);
		f->jobs = realloc(f->jobs,(f->njobs+1)*sizeof *f->jobs);
		f->jobs[f->njobs] = strdup(job);
		f->job_attrs = realloc(f->job_attrs,(f->njobs+1)*sizeof *f->job_attrs);
		f->job_attrs[f->njobs] = out;
		out = NULL;

		for (oi=0; strcmp(meta[oi].name,GLITE_JP_ATTR_OWNER); oi++);
		assert(meta[oi].name);
		f->owners = realloc(f->owners,(f->njobs+1)*sizeof *f->owners);
		f->owners[f->njobs] = strdup(meta[oi].value);

		f->njobs++;
	}

/* run the feed eventually */
	if (f->njobs >= BATCH_FEED_SIZE && drain_feed(ctx,f,0)) {
		err.code = EIO;
		err.desc = "sending batch feed";
		glite_jp_stack_error(ctx,&err);
	}

cleanup:
	for (i=0; other && other[i].name; i++) glite_jp_attrval_free(other+i,0);
	free(other);

	return err.code;
}


static int run_feed_deferred(glite_jp_context_t ctx,void *feed)
{
	struct jpfeed	*f = feed;
	int	i,m,o,cnt,ret = 0;
	char	**meta;

	glite_jp_clear_error(ctx);
/* count "meta" attributes */
	cnt = 0;
	for (i=0; f->attrs[i]; i++)
		if (glite_jppsbe_is_metadata(ctx,f->attrs[i])) cnt++;

	f->meta_attr = cnt ? malloc((cnt+1) * sizeof *f->meta_attr) : NULL;
	f->nmeta_attr = cnt;

	f->other_attr = i-cnt ? malloc((i-cnt+1) * sizeof *f->other_attr) : NULL;
	f->nother_attr = i-cnt;

/* sort attributes to "meta" and others */
	m = o = 0;
	for (i=0; f->attrs[i]; i++)
		if (glite_jppsbe_is_metadata(ctx,f->attrs[i])) 
			if (!strcmp(f->attrs[i],GLITE_JP_ATTR_OWNER)) {
				free(f->attrs[i]);
				f->nmeta_attr--;
			}
			else f->meta_attr[m++] = f->attrs[i];
		else {
			/* XXX: jobid and owner are sent anyway */
			if (!strcmp(f->attrs[i],GLITE_JP_ATTR_JOBID)) {
				free(f->attrs[i]);
				f->nother_attr--;
			}
			else f->other_attr[o++] = f->attrs[i];
		}

	if (f->nmeta_attr == 0) { free(f->meta_attr); f->meta_attr = NULL; }
	if (f->nother_attr == 0) { free(f->other_attr); f->other_attr = NULL; }

	if (f->meta_attr) f->meta_attr[m] = NULL;
	if (f->other_attr) f->other_attr[o] = NULL;


/* the same for query records */
	cnt = 0;
	for (i=0; f->qry[i].attr; i++)
		if (glite_jppsbe_is_metadata(ctx,f->qry[i].attr)) cnt++;

	f->meta_qry = cnt ? malloc((cnt+1) * sizeof *f->meta_qry) : NULL;
	if (f->meta_qry) memset(f->meta_qry+cnt,0,sizeof *f->meta_qry);
	f->nmeta_qry = cnt;
	f->other_qry = i-cnt ? malloc((i-cnt+1) * sizeof *f->other_qry) : NULL;
	f->nother_qry = i-cnt;

	m = o = 0;
	for (i=0; f->qry[i].attr; i++) 
		if (glite_jppsbe_is_metadata(ctx,f->qry[i].attr)) memcpy(f->meta_qry+m++,f->qry+i,sizeof *f->meta_qry);
		else memcpy(f->other_qry+o++,f->qry+i,sizeof *f->other_qry);

	free(f->attrs); free(f->qry);
	f->attrs = NULL;
	f->qry = NULL;

	if (f->meta_qry) memset(f->meta_qry+m,0,sizeof *f->meta_qry);
	else {
		glite_jp_error_t	err;
		err.code = EINVAL;
		err.source = __FUNCTION__;
		err.desc = "at least one metadata query item required";
		ret = glite_jp_stack_error(ctx,&err);
		goto cleanup;
	}
		
	if (f->other_qry) memset(f->other_qry+o,0,sizeof *f->other_qry);

/* extract other_qry items that are not present in other_attr */
	f->int_other_attr = o = f->nother_attr;
	for (i=0; i<f->nother_qry; i++) {
		int	j;
		for (j=0; j<f->int_other_attr && strcmp(f->other_attr[j],f->other_qry[i].attr); j++);
		if (j == f->int_other_attr) {
			f->other_attr = realloc(f->other_attr,(o+2) * sizeof *f->other_attr);
			f->other_attr[o++] = strdup(f->other_qry[i].attr);
		}
	}
	if (f->other_attr) f->other_attr[o] = NULL;
	f->nother_attr = o;

	meta = calloc(f->nmeta_attr+2,sizeof *meta);
	meta[0] = GLITE_JP_ATTR_OWNER;
	if (f->meta_attr) memcpy(meta+1,f->meta_attr,(f->nmeta_attr+1) * sizeof *meta);
	else meta[1] = NULL;

	ret = glite_jppsbe_query(ctx,f->meta_qry,meta,f,feed_query_callback);
	if (!ret) ret = drain_feed(ctx,f,1);
	else drop_jobs(f);

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

/* FIXME:
 * - volatile implementation: should store the registrations in a file
 *   and recover after restart
 * - should communicate the data among all server slaves

	f->next = ctx->feeds;
	ctx->feeds = f;
 */

	if (glite_jppsbe_store_feed(ctx,f)) fputs(glite_jp_error_chain(ctx),stderr);
	else kill(-master,SIGUSR1);	/* gracefully terminate slaves 
				   and let master restart them */

	return 0;
}


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

