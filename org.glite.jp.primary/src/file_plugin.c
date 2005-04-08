#include <stdio.h>
#include <getopt.h>
#include <dlfcn.h>
#include <errno.h>

#include <glite/jp/types.h>
#include "file_plugin.h"

static struct option opts[] = {
	{ "plugin", 1, NULL, 'p' },
	{ NULL }
};

static void *loadit(glite_jp_context_t ctx,const char *so)
{
	void	*dl_handle = dlopen(so,RTLD_NOW);
	glite_jp_error_t	err;
	char	*e;

	glite_jpps_fplug_init_t	init;

	if (!dl_handle) {
		err.source = "dlopen()";
		err.code = EINVAL;
		err.desc = dlerror();
		glite_jp_stack_error(ctx,&err);
		return NULL;
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
	}

	/* FIXME: zavolat init */

}

int glite_jpps_fplug_load(glite_jp_context_t ctx,int *argc,char **argv)
{
	int	opt;
	void	*fctx;

	while ((opt = getopt_long(*argc,argv,"p:",opts,NULL)) != EOF) switch (opt) {
		case 'p': fctx = loadit(ctx,optarg);
			if (!fctx) return -1;
		default: break;
	}

	return 0;
}

int glite_jpps_fplug_lookup(glite_jp_context_t ctx,const char *uri, glite_jpps_fplug_data_t *plugin_data)
{
	/* TODO */
	return 0;
}

