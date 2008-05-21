#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>

#include <glite/jp/types.h>
#include "glite/jp/type_plugin.h"

int glite_jp_typeplugin_load(glite_jp_context_t ctx,const char *so){
/* XXX: not stored but we never dlclose() yet */
        void    *dl_handle = dlopen(so,RTLD_NOW);

        glite_jp_error_t        err;
        const char      *e;
        glite_jp_tplug_data_t *data;
        int     i;

        glite_jp_tplug_init_t init;
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
                char    buf[300];
                snprintf(buf,sizeof buf,"dlsym(\"%s\",\"init\")",so);
                buf[299] = 0;
                err.source = buf;
                err.code = ENOENT;
                err.desc = e;
                return glite_jp_stack_error(ctx,&err);
        }

        data = calloc(1,sizeof *data);

        if (init(ctx, NULL, data)) return -1;

        i = 0;
        if (ctx->plugins) for (i=0; ctx->plugins[i]; i++);
        ctx->plugins = realloc(ctx->plugins, (i+2) * sizeof *ctx->plugins);
        ctx->plugins[i] = data;
        ctx->plugins[i+1] = NULL;

        /* TODO: check consistency of uri+class pairs wrt. previous plugins */

        return 0;
}

