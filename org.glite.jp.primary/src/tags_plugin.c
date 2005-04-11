#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include <glite/jp/types.h>

#include "file_plugin.h"
#include "builtin_plugins.h"

static int tagappend(void *,void *,int,...);
static int tagopen(void *,void *,void **);
static int tagclose(void *,void *);

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
	data->ops.attr = tagdummy;
	data->ops.generic = tagappend;
	
	printf("tags init OK\n");
	return 0;
}

static int tagopen(void *fpctx,void *bhandle,void **handle)
{
	/* we don't need anything special yet, so just pass the backend handle */
	*handle = bhandle;
	return 0;
}

static int tagclose(void *fpctx,void *handle)
{
	return 0;
}

static int tagappend(void *fpctx,void *handle,int oper,...)
{
	glite_jp_tagval_t	*tag;
	va_list	ap;
	va_start(ap,oper);
	tag = va_arg(ap,glite_jp_tagval_t *);
	va_end(ap);

	/* TODO */
	printf("tagappend: %s,%d,%s\n",tag->name,tag->sequence,tag->value);
	
	return 0;
}
