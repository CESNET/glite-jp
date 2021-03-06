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

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <cclassad.h>
#include <errno.h>
#include <sys/stat.h>

/*
#include "glite/lb/context.h"
#include "glite/lb/jobstat.h"
#include "glite/lb/events.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/trio.h"
*/
#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/attr.h"
#include "glite/jp/known_attr.h"

#include "glite/jp/file_plugin.h"
#include "glite/jp/backend.h"
#include "glite/jp/builtin_plugins.h"

//#define INITIAL_NUMBER_EVENTS 100
//#define INITIAL_NUMBER_STATES EDG_WLL_NUMBER_OF_STATCODES
//#define LB_PLUGIN_NAMESPACE "urn:org.glite.lb"

//extern int processEvent(intJobStat *, edg_wll_Event *, int, int, char **);


typedef struct _classad_handle{
	char* data;
	struct cclassad* ad;
	time_t timestamp;
} classad_handle;

static int classad_query(void *fpctx, void *handle, const char *attr, glite_jp_attrval_t **attrval);
static int classad_open(void *fpctx, void *bhandle, const char *uri, void **handle);
static int classad_open_str(void *fpctx, const char *str, const char *uri, const char *ns, void **handle);
static int classad_close(void *fpctx, void *handle);
static int classad_filecom(void *fpctx, void *handle);

int init(glite_jp_context_t ctx, glite_jpps_fplug_data_t *data) {
	data->fpctx = ctx;

	data->uris = calloc(2,sizeof *data->uris);
	data->uris[0] = strdup(GLITE_JP_FILETYPE_CLASSAD);

	data->classes = calloc(2,sizeof *data->classes);
	data->classes[0] = strdup("classad");

	data->namespaces = calloc(2, sizeof *data->namespaces);
	data->namespaces[0] = strdup(GLITE_JP_JDL_NS);

	data->ops.open 	= classad_open;
	data->ops.close = classad_close;
	data->ops.attr 	= classad_query;
	data->ops.open_str = classad_open_str;
	data->ops.filecom = classad_filecom;

#ifdef PLUGIN_DEBUG
        fprintf(stderr,"classad_plugin: init OK\n");
#endif
	
	return 0;
}


void done(glite_jp_context_t ctx, glite_jpps_fplug_data_t *data) {
	free(data->uris[0]);
	free(data->classes[0]);
	free(data->uris);
	free(data->classes);
	memset(data, 0, sizeof(*data));
}


static int classad_open(void *fpctx, void *bhandle, const char *uri, void **handle) {
	glite_jp_context_t  ctx = (glite_jp_context_t) fpctx;
	glite_jp_error_t    err;
	classad_handle*     h;
	void*		    fh;
	int		    retval = 0;

	glite_jp_clear_error(ctx);
	h = calloc(1, sizeof(classad_handle));
	h->data = NULL;
	struct stat fattr;
	glite_jppsbe_file_attrs(ctx, bhandle, &fattr);
	h->timestamp = fattr.st_mtime;

	// read the classad file
	char buf[1024];
	size_t nbytes;
	off_t offset = 0;

	do{
		if (! (retval = glite_jppsbe_pread(ctx, bhandle, buf, sizeof buf, offset, &nbytes))){
			h->data = realloc(h->data, offset + nbytes);
			memcpy(h->data + offset, buf, nbytes);
			offset += nbytes;
		}
		else
			goto fail;
	}while(nbytes);

	h->ad = cclassad_create(h->data);

#ifdef PLUGIN_DEBUG
        fprintf(stderr,"classad_plugin: opened\n");
#endif

	*handle = h;	

	return 0;

fail:
	err.code = EIO;
	err.desc = NULL;
	err.source = __FUNCTION__;
	glite_jp_stack_error(ctx,&err);

	return retval;
}

static int classad_open_str(void *fpctx,const char *str,const char *uri,const char *ns,void **handle){
        classad_handle*     h;
	
	h = calloc(1, sizeof(classad_handle));
        h->data = strdup(str);
	h->ad = cclassad_create(h->data);
	h->timestamp = 0;

#ifdef PLUGIN_DEBUG
        fprintf(stderr,"classad_plugin: opened\n");
#endif

        *handle = h;

        return 0;

}

static int classad_close(void *fpctx,void *handle) {
	classad_handle *h = (classad_handle *) handle;

	cclassad_delete(h->ad);
	free(h->data);
	free(h);

#ifdef PLUGIN_DEBUG
	fprintf(stderr,"classad plugin: close OK\n");
#endif
	return 0;
}


static int classad_query(void *fpctx,void *handle, const char *attr,glite_jp_attrval_t **attrval) {
	glite_jp_context_t	ctx = (glite_jp_context_t) fpctx;
	glite_jp_error_t 	err;
	glite_jp_attrval_t      *av = NULL;
	classad_handle*     	h = (classad_handle*)handle;

        glite_jp_clear_error(ctx); 
        memset(&err,0,sizeof err);
        err.source = __FUNCTION__;

	char *str = NULL;

	if (! h->ad){
		err.code = ENOENT;
                err.desc = strdup("Classad plugin: No classad string, cannot get attr!");
		*attrval = NULL;
		printf("Exiting classat_query...\n");
                return glite_jp_stack_error(ctx,&err);
	}

	if (!cclassad_evaluate_to_string(h->ad, strrchr(attr, ':')+1, &str) &&
		cclassad_evaluate_to_expr(h->ad, strrchr(attr, ':')+1, &str) &&
			!strcasecmp(str,"undefined")) { free(str); str = NULL; }

	if (str) {
        	//struct stat fattr;
		/*XXX ignore error */
		//glite_jppsbe_file_attrs(ctx, h->bhandle, &fattr);
		av = calloc(2, sizeof(glite_jp_attrval_t));
                av[0].name = strdup(attr);
                av[0].value = str; str = NULL;
		av[0].size = -1;
                av[0].timestamp = h->timestamp;
		av[0].origin = GLITE_JP_ATTR_ORIG_FILE;
        }
	else{
		printf("Classad plugin: bad attr!\n");
	}
        
        if (str) free(str);

	*attrval = av;

	if (av)
		return 0;
	else{
		err.code = ENOENT;
                err.desc = attr;
		return glite_jp_stack_error(ctx,&err);
	}
}

static int classad_filecom(void *fpctx, void *handle){
	return -1;
}
 
