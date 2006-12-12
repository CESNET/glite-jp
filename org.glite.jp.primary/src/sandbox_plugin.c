#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <libtar.h>
#include <fcntl.h>

#include <glite/jp/types.h>

#include "file_plugin.h"
#include "builtin_plugins.h"
#include "backend.h"

typedef struct _sb_handle {
	void    	*bhandle;
	TAR             *t;
	tartype_t	*tt;
	char		**file_names;
} sb_handle;

// Global data needed for read/write wrappers
static struct {
	void			*bhandle;
	glite_jp_context_t	ctx;
	off_t			offset;
} global_data;
		

//static int sandbox_append(void *,void *,int,...);
static int sandbox_open(void *,void *,const char *uri,void **);
static int sandbox_close(void *,void *);
static int sandbox_attr(void *,void *,const char *,glite_jp_attrval_t **);

static int my_open(const char *pathname, int flags, ...) {
	// Do not open file, it is opened in ftp_backend
	// returned fd does not matter, read/write/close does ftp_backend
	return 12345;
}

static int my_close(int fd) {
	// Closed in ftp_backend
        return 0;
}

// wrapper around glite_jppsbe_pread
static ssize_t my_read(int fd, void *buf, size_t count) 
{
	size_t                  r;

	
	if (glite_jppsbe_pread(global_data.ctx,global_data.bhandle,buf,count,global_data.offset,&r)) {
		errno = global_data.ctx->error->code;
		return -1;
	}

	global_data.offset += r;

	return r;
}


static ssize_t my_write(int fd, const void *buf, size_t count) {
	// XXX: wrapper around glite_jppsbe_pwrite
}


int init(glite_jp_context_t ctx, glite_jpps_fplug_data_t *data)
{
	data->fpctx = ctx;
	global_data.ctx = ctx;

	data->uris = calloc(2,sizeof *data->uris);
	data->uris[0] = strdup(GLITE_JP_FILETYPE_ISB);

	data->classes = calloc(2,sizeof *data->classes);
	data->classes[0] = strdup("sandbox");

	data->ops.open = sandbox_open;
	data->ops.close = sandbox_close;
	data->ops.attr = sandbox_attr;
	
	printf("sandbox_plugin: URI: \"%s\"\n",GLITE_JP_FILETYPE_ISB);
	return 0;
}

static int sandbox_open(void *fpctx,void *bhandle,const char *uri,void **handle)
{
	sb_handle 	*h = calloc(1,sizeof *h);


	printf("sandbox_open() called\n");

	h->bhandle = bhandle;
	global_data.bhandle = bhandle;
	global_data.offset = 0;

	h->tt = malloc(sizeof(*h->tt));
	h->tt->openfunc = my_open;
	h->tt->closefunc = my_close;
	h->tt->readfunc = my_read;
	h->tt->writefunc = my_write;

	if (tar_open(&h->t, NULL /* not needed, opened in ftp_backend */, h->tt, O_RDONLY, 0, TAR_GNU) == -1)
		printf("tar_open()\n"); //XXX: use glite_jp_stack_error

//	tar_fdopen(&t, ((fhandle)handle)->fd, char *pathname, tartype_t *type, int oflags, int mode, int options);


	*handle = h;

	return 0;
}

static int sandbox_close(void *fpctx,void *handle)
{
	int	i;
	sb_handle *h = handle;

	tar_close(h->t);
	free(h->tt);

	for (i=0; h->file_names; i++) free(h->file_names[i]);
	free(h->file_names);
	
	free(h);

	printf("sandbox_close() called\n");

	return 0;
}


static int sandbox_attr(void *fpctx,void *handle,const char *attr,glite_jp_attrval_t **attrval)
{
	glite_jp_error_t	err;
	glite_jp_context_t	ctx = fpctx;
	glite_jp_attrval_t	*out = NULL;
	int			i,nout = 0;
	sb_handle 	*h = handle;

	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	glite_jp_clear_error(ctx);


	while ((i = th_read(h->t)) == 0)
	{
		printf("-- %s\n", th_get_pathname(h->t));

		if (TH_ISREG(h->t) && tar_skip_regfile(h->t) != 0)
		{
			err.code = EIO;
			err.desc = "tar_skip_regfile";
			return glite_jp_stack_error(ctx,&err);
		}
	}

	printf("sandbox_attr() called\n");
}

