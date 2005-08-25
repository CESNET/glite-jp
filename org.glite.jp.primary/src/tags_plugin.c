#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <glite/jp/types.h>

#include "file_plugin.h"
#include "builtin_plugins.h"
#include "backend.h"

static int tagappend(void *,void *,int,...);
static int tagopen(void *,void *,const char *uri,void **);
static int tagclose(void *,void *);
static int tagattr(void *,void *,const char *,glite_jp_attrval_t **);

struct tags_handle {
	void	*bhandle;
	int	n;
	glite_jp_attrval_t	*tags;
};

static int tagsread(void *,struct tags_handle *);

#define TAGS_MAGIC 0x74c016f2	/* two middle digits encode version, i.e. 01 */

static int tagdummy()
{
	puts("tagdummy()");
	return -1;
}

int init(glite_jp_context_t ctx, glite_jpps_fplug_data_t *data)
{
	data->fpctx = ctx;

	data->uris = calloc(2,sizeof *data->uris);
	data->uris[0] = strdup(GLITE_JP_FILETYPE_TAGS);

	data->classes = calloc(2,sizeof *data->classes);
	data->classes[0] = strdup("tags");

	data->ops.open = tagopen;
	data->ops.close = tagclose;
	data->ops.attr = tagattr;
	data->ops.generic = tagappend;
	
	printf("tags_plugin: URI: \"%s\"; magic number: 0x%08lx\n",GLITE_JP_FILETYPE_TAGS,TAGS_MAGIC);
	return 0;
}

static int tagopen(void *fpctx,void *bhandle,const char *uri,void **handle)
{
	struct tags_handle *h = calloc(1,sizeof *h);
	h->n = 0;
	h->bhandle = bhandle;

	*handle = h;

	return 0;
}

static int tagclose(void *fpctx,void *handle)
{
	int	i;
	struct tags_handle *h = handle;

	for (i=0; i<h->n; i++) glite_jp_attrval_free(h->tags+i,0);
	free(h->tags);
	free(h);

	return 0;
}

static int tagappend(void *fpctx,void *handle,int oper,...)
{
	glite_jp_attrval_t	*tag;
	va_list	ap;
	char	*hdr,*rec;
	glite_jp_context_t	ctx = fpctx;
	struct tags_handle	*h = handle;
	uint32_t		magic,hlen,rlen,rlen_n;
	size_t			r;
	glite_jp_error_t	err;

	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	glite_jp_clear_error(ctx);

	va_start(ap,oper);
	tag = va_arg(ap,glite_jp_attrval_t *);
	va_end(ap);

	printf("tagappend: %s,%s\n",tag->name,tag->value);

	assert(oper == GLITE_JP_FPLUG_TAGS_APPEND);

	if (glite_jppsbe_pread(ctx,h->bhandle,&magic,sizeof magic,0,&r)) {
		err.code = EIO;
		err.desc = "reading magic number";
		return glite_jp_stack_error(ctx,&err);
	}

	if (r == 0) {
		magic = htonl(TAGS_MAGIC);
		if (glite_jppsbe_pwrite(ctx,h->bhandle,&magic,sizeof magic,0)) {
			err.code = EIO;
			err.desc = "writing magic number";
			return glite_jp_stack_error(ctx,&err);
		}
	}
	else if (r != sizeof magic) {
		err.code = EIO;
		err.desc = "can't read magic number";
		return glite_jp_stack_error(ctx,&err);
	}
	else if (magic != htonl(TAGS_MAGIC)) {
		err.code = EINVAL;
		err.desc = "invalid magic number";
		return glite_jp_stack_error(ctx,&err);
	}

/* XXX: origin is always USER, not recorded */
	trio_asprintf(&hdr,"%ld %c",
			tag->timestamp,tag->binary ? 'B' : 'S');

	rlen = strlen(tag->name) + strlen(hdr) + 2 /* \0 after name and after hdr */ +
		(r = tag->binary ? tag->size : (tag->value ? strlen(tag->value) : 0));

	rlen_n = htonl(rlen);

	rec = malloc(rlen + sizeof rlen_n);
	*((uint32_t *) rec) = rlen_n;
	strcpy(rec + sizeof rlen_n,tag->name);
	strcpy(rec + (hlen = sizeof rlen_n + strlen(tag->name) + 1),hdr);

	if (r) memcpy(rec + hlen + strlen(hdr) + 1,tag->value,r);
	free(hdr);

/* record format:
 * - 4B length, net byte order
 * - attr name, \0
 * - %ld %c \0 (timestamp, B/S)
 * - value
 */
	if (glite_jppsbe_append(ctx,h->bhandle,rec,rlen + sizeof rlen_n)) {
		err.code = EIO;
		err.desc = "writing tag record";
		free(rec);
		return glite_jp_stack_error(ctx,&err);
	}

	/* XXX: should add tag also to handle->tags, but it is never used 
	 * currently */
	
	return 0;
}

static int tagattr(void *fpctx,void *handle,const char *attr,glite_jp_attrval_t **attrval)
{
	struct tags_handle	*h = handle;
	glite_jp_error_t	err;
	glite_jp_context_t	ctx = fpctx;
	glite_jp_attrval_t	*out = NULL;
	int	i,nout = 0;

	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	if (!h->tags) tagsread(fpctx,handle);

	if (!h->tags) {
		err.code = ENOENT;
		err.desc = "no tags for this job";
		return glite_jp_stack_error(ctx,&err);
	}

	for (i=0; i<h->n; i++) if (!strcmp(h->tags[i].name,attr)) {
		out = realloc(out,(nout+2) * sizeof *out);
		glite_jp_attrval_copy(out+nout,h->tags+i);
		nout++;
		memset(out+nout,0,sizeof *out);
	}

	if (nout) {
		*attrval = out;
		return 0;
	}
	else {
		err.code = ENOENT;
		err.desc = "no value for this tag";
		return glite_jp_stack_error(ctx,&err);
	}
}

static int tagsread(void *fpctx,struct tags_handle *h)
{
	glite_jp_context_t	ctx = fpctx;
	uint32_t		magic,rlen;
	glite_jp_error_t	err;
	int			r;
	size_t			off = sizeof rlen;
	glite_jp_attrval_t	*tp;
	char			*rp;

	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	
	glite_jp_clear_error(ctx);

/* read magic number */
	if (glite_jppsbe_pread(ctx,h->bhandle,&magic,sizeof magic,0,&r)) {
		err.code = EIO;
		err.desc = "reading magic number";
		return glite_jp_stack_error(ctx,&err);
	}

	if (r != sizeof magic) {
		err.code = EIO;
		err.desc = "can't read magic number";
		return glite_jp_stack_error(ctx,&err);
	}
	else if (magic != htonl(TAGS_MAGIC)) {
		err.code = EINVAL;
		err.desc = "invalid magic number";
		return glite_jp_stack_error(ctx,&err);
	}


	while (1) {
		char	*rec,type;
		int	rd;

	/* read record header */
		if (glite_jppsbe_pread(ctx,h->bhandle,&rlen,sizeof rlen,off,&r)) {
			err.code = EIO;
			err.desc = "reading record header";
			return glite_jp_stack_error(ctx,&err);
		}
		if (r == 0) break;

		if (r != sizeof rlen) {
			err.code = EIO;
			err.desc = "can't read record header";
			return glite_jp_stack_error(ctx,&err);
		}

		off += r;
		rec = malloc(rlen = ntohl(rlen));

	/* read whole record body thoroughly */
		for (rd=0; rd<rlen; rd+=r) /* XXX: will loop on 0 bytes read */
			if (glite_jppsbe_pread(ctx,h->bhandle,rec+rd,rlen-rd,off+rd,&r)) {
				err.code = EIO;
				err.desc = "reading record body";
				free(rec);
				return glite_jp_stack_error(ctx,&err);
			}

		off += rlen;

	/* parse the record */
		h->tags = realloc(h->tags,(h->n+2) * sizeof *h->tags);
		tp = h->tags+h->n++;
		memset(tp,0,sizeof *tp);

		tp->name = strdup(rec);
		rp = rec + strlen(rec) + 1;

		sscanf(rp,"%ld %c",&tp->timestamp,&type);
		rp += strlen(rp) + 1;
		switch (type) {
			int	i;

			case 'B': tp->binary = 1; break;
			case 'S': tp->binary = 0; break;
			default: free(rec);
				 for (i=0; i<h->n; i++)
					 glite_jp_attrval_free(h->tags+i,0);
				 free(h->tags);
				 h->tags = NULL;
				 h->n = 0;

				 err.code = EINVAL;
				 err.desc = "invalid attr type (B/S)";
				 return glite_jp_stack_error(ctx,&err);
		}
		tp->value = malloc((r=rlen - (rp - rec)) + 1);
		memcpy(tp->value,rp,r);
		if (!tp->binary) tp->value[r] = 0;
		tp->origin = GLITE_JP_ATTR_ORIG_USER;

		free(rec);
	}
	return 0;
}
