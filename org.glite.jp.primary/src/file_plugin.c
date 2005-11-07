#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <dlfcn.h>
#include <errno.h>

#include <glite/jp/types.h>
#include "file_plugin.h"

static struct option opts[] = {
	{ "plugin", 1, NULL, 'p' },
	{ NULL }
};

static int loadit(glite_jp_context_t ctx,const char *so)
{
/* XXX: not stored but we never dlclose() yet */
	void	*dl_handle = dlopen(so,RTLD_NOW);

	glite_jp_error_t	err;
	const char	*e;
	glite_jpps_fplug_data_t	*data,*dp;
	int	i;

	glite_jpps_fplug_init_t	init;
	memset(&err,0,sizeof err);

	if (!dl_handle) {
		err.source = "dlopen()";
		err.code = EINVAL;
		err.desc = dlerror();
		return glite_jp_stack_error(ctx,&err);
	}

	dlerror();
	init = dlsym(dl_handle,"init");
	e = dlerror();
	if (e) {
		char	buf[300];
		snprintf(buf,sizeof buf,"dlsym(\"%s\",\"init\")",so);
		buf[299] = 0;
		err.source = buf;
		err.code = ENOENT;
		err.desc = e;
		return glite_jp_stack_error(ctx,&err);
	}

	data = calloc(1,sizeof *data);

	if (init(ctx,data)) return -1;

	i = 0;
	if (ctx->plugins) for (i=0; ctx->plugins[i]; i++);
	ctx->plugins = realloc(ctx->plugins, (i+2) * sizeof *ctx->plugins);
	ctx->plugins[i] = data;
	ctx->plugins[i+1] = NULL;

	/* TODO: check consistency of uri+class pairs wrt. previous plugins */
	
	return 0;
}

int glite_jpps_fplug_load(glite_jp_context_t ctx,int argc,char **argv)
{
	int	i;

	for (i=1; i<argc; i++) if (loadit(ctx,argv[i])) {
		glite_jp_error_t	err;
		memset(&err,0,sizeof err);
		err.source = __FUNCTION__;
		err.code = EINVAL;
		err.desc = argv[i];
		return glite_jp_stack_error(ctx,&err);
	}

	return 0;
}

int glite_jpps_fplug_lookup(glite_jp_context_t ctx,const char *uri, glite_jpps_fplug_data_t ***plugin_data)
{
	int	i;

	glite_jpps_fplug_data_t	**out = NULL;
	int	matches = 0;

	glite_jp_error_t	err;
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	err.code = ENOENT;
	err.desc = (char *) uri;	/* XXX: we don't modify it, believe me, gcc! */

	glite_jp_clear_error(ctx);
	if (!ctx->plugins) {
		return glite_jp_stack_error(ctx,&err);
	}

	for (i = 0; ctx->plugins[i]; i++) {
		int	j;
		glite_jpps_fplug_data_t	*p = ctx->plugins[i];

		for (j=0; p->uris && p->uris[j]; j++)
			if (!strcmp(p->uris[j],uri)) {
				out = realloc(out, (matches+2) * sizeof *out);
				out[matches++] = p;
				out[matches] = NULL;
			}
	}

	if (matches) {
		*plugin_data = out;
		return 0;
	}
	else return glite_jp_stack_error(ctx,&err);
}
