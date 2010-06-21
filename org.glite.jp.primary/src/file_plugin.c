/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners/ for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <dlfcn.h>
#include <errno.h>

#include <glite/jp/types.h>
#include "glite/jp/file_plugin.h"

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

static int lookup_common(glite_jp_context_t ctx,const char *uri,const char *class, glite_jpps_fplug_data_t ***plugin_data)
{
	int	i;

	glite_jpps_fplug_data_t	**out = NULL;
	int	matches = 0;

	glite_jp_error_t	err;
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	err.code = ENOENT;
	err.desc = (char *) (uri ? uri : class);	/* XXX: we don't modify it, believe me, gcc! */

	glite_jp_clear_error(ctx);
	if (!ctx->plugins) {
		return glite_jp_stack_error(ctx,&err);
	}

	for (i = 0; ctx->plugins[i]; i++) {
		int	j;
		glite_jpps_fplug_data_t	*p = ctx->plugins[i];

		for (j=0; p->uris && p->uris[j]; j++)
			if ((uri && !strcmp(p->uris[j],uri)) || (class && !strcmp(p->classes[j],class))) {
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

int glite_jpps_fplug_lookup(glite_jp_context_t ctx,const char *uri, glite_jpps_fplug_data_t ***plugin_data)
{
	return lookup_common(ctx,uri,NULL,plugin_data);
}

int glite_jpps_fplug_lookup_byclass(glite_jp_context_t ctx, const char *class,glite_jpps_fplug_data_t ***plugin_data)
{
	return lookup_common(ctx,NULL,class,plugin_data);
}

