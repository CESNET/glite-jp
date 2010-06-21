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
#include <string.h>
#include <dlfcn.h>
#include <errno.h>

#include <glite/jp/types.h>
#include <glite/jp/context.h>
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
        if (ctx->type_plugins) for (i=0; ctx->type_plugins[i]; i++);
        ctx->type_plugins = realloc(ctx->type_plugins, 
		(i+2) * sizeof *ctx->type_plugins);
        ctx->type_plugins[i] = data;
        ctx->type_plugins[i+1] = NULL;

        /* TODO: check consistency of uri+class pairs wrt. previous plugins */

        return 0;
}

