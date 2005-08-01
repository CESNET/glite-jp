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

#define TAGS_MAGIC 0x74c016f2	/* two middle digits encode version, i.e. 01 */

static int tagdummy()
{
	puts("tagdummy()");
	return -1;
}

struct tags_handle {
	void	*bhandle;
	int	n;
	glite_jp_tagval_t	*tags;
};

int init(glite_jp_context_t ctx, glite_jpps_fplug_data_t *data)
{
	data->fpctx = ctx;

	data->uris = calloc(2,sizeof *data->uris);
	data->uris[0] = strdup(GLITE_JP_FILETYPE_TAGS);

	data->classes = calloc(2,sizeof *data->classes);
	data->classes[0] = strdup("tags");

	data->ops.open = tagopen;
	data->ops.close = tagclose;
	data->ops.attr = tagdummy;
	data->ops.generic = tagappend;
	
	printf("tags_plugin: URI: \"%s\"; magic number: 0x%08lx\n",GLITE_JP_FILETYPE_TAGS,TAGS_MAGIC);
	return 0;
}

static int tagopen(void *fpctx,void *bhandle,const char *uri,void **handle)
{
	struct tags_handle *h = calloc(1,sizeof *h);
	h->n = -1;
	h->bhandle = bhandle;

	*handle = h;

	return 0;
}

static int tagclose(void *fpctx,void *handle)
{
	int	i;
	struct tags_handle *h = handle;

	for (i=0; i<h->n; i++) {
		free(h->tags[i].name);
		free(h->tags[i].value);
	}
	free(h->tags);
	free(h);

	return 0;
}

static int tagappend(void *fpctx,void *handle,int oper,...)
{
	glite_jp_tagval_t	*tag;
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
	tag = va_arg(ap,glite_jp_tagval_t *);
	va_end(ap);

	printf("tagappend: %s,%d,%s\n",tag->name,tag->sequence,tag->value);

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

	trio_asprintf(&hdr,"%d %ld %c",tag->sequence,
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

	if (glite_jppsbe_append(ctx,h->bhandle,rec,rlen + sizeof rlen_n)) {
		err.code = EIO;
		err.desc = "writing tag record";
		free(rec);
		return glite_jp_stack_error(ctx,&err);
	}
	
	return 0;
}
