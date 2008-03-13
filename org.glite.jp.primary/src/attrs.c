#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "glite/jp/types.h"
#include "glite/jp/attr.h"

#include "feed.h"
#include "backend.h"
#include "attrs.h"
#include "utils.h"
#include "file_plugin.h"
#include "builtin_plugins.h"

static struct {
	char *namespace;
	glite_jpps_fplug_data_t **plugins;
        int     nplugins;
	char **opened_classes;          // for each plugin contains name of the open class (NULL for no open file)
	char **opened_files;		// for each plugin contains name of the open file (NULL for no open file)
	void **plugin_handles;		// contains handle for each opened plugin (NULL for not opened)
	void **file_handles;		// contains handle for each opened file by plugin (NULL for not opened)
} *known_namespaces;

static void scan_namespaces(glite_jp_context_t ctx)
{
        int     i,j,k;
        glite_jpps_fplug_data_t *pd;

	if (!ctx->plugins) return;
        
	for (i=0; ctx->plugins[i]; i++) {
                pd = ctx->plugins[i];
		
		if (pd->namespaces){
	                for (j=0; pd->namespaces[j]; j++) {
				for (k=0; known_namespaces && known_namespaces[k].namespace
        	        	                       && strcmp(pd->namespaces[j],known_namespaces[k].namespace); k++) {};
			
				if (known_namespaces && known_namespaces[k].namespace) {
					printf("Adding new plugin into namespace %s\n", known_namespaces[k].namespace);
                                	known_namespaces[k].plugins = realloc(known_namespaces[k].plugins,
                               			(known_namespaces[k].nplugins + 2) * sizeof(glite_jpps_fplug_data_t *));
	                               	known_namespaces[k].plugins[known_namespaces[k].nplugins++] = pd;
	        	                known_namespaces[k].plugins[known_namespaces[k].nplugins] = NULL;
					known_namespaces[k].namespace = pd->namespaces[j];
					known_namespaces[k].opened_classes = realloc(known_namespaces[k].opened_classes,
						(known_namespaces[k].nplugins + 1) * sizeof(char*));
					known_namespaces[k].opened_classes[known_namespaces[k].nplugins-1] = NULL;
					known_namespaces[k].opened_files = realloc(known_namespaces[k].opened_files,
                                                (known_namespaces[k].nplugins + 1) * sizeof(char*));
					known_namespaces[k].opened_files[known_namespaces[k].nplugins-1] = NULL;
					known_namespaces[k].plugin_handles = realloc(known_namespaces[k].plugin_handles,
						(known_namespaces[k].nplugins + 1) * sizeof(void*));
					known_namespaces[k].plugin_handles[known_namespaces[k].nplugins-1] = NULL;
					known_namespaces[k].file_handles = realloc(known_namespaces[k].file_handles,
                                                (known_namespaces[k].nplugins + 1) * sizeof(void*));
                                        known_namespaces[k].file_handles[known_namespaces[k].nplugins-1] = NULL;
                	       	}
	                        else {
					printf("Adding new namespace %s\n", pd->namespaces[j]);
        	               	        known_namespaces = realloc(known_namespaces,(k+2) * sizeof *known_namespaces);
        	                       	known_namespaces[k].plugins = malloc(2 * sizeof(glite_jpps_fplug_data_t *));
	        	                known_namespaces[k].plugins[0] = pd;
        	                       	known_namespaces[k].plugins[1] = NULL;
                	       	        known_namespaces[k].nplugins = 1;
					known_namespaces[k].namespace = pd->namespaces[j];
                        	        memset(known_namespaces+k+1,0,sizeof *known_namespaces);
					known_namespaces[k].opened_classes = calloc(1, sizeof(char*));
					known_namespaces[k].opened_files = calloc(1, sizeof(char*));
					known_namespaces[k].plugin_handles = calloc(1, sizeof(void*));
					known_namespaces[k].file_handles = calloc(1, sizeof(void*));
       		                }
	               	}
		}
        }
}

static int merge_attrvals(glite_jp_attrval_t **out,int nout,const glite_jp_attrval_t *in)
{
	int	nin;
	
	if (!in) return nout;

	for (nin=0; in[nin].name; nin++);
	*out = realloc(*out,(nout+nin+1) * sizeof **out);
	memcpy(*out + nout,in,(nin+1) * sizeof **out);
	memset(*out + nout+nin, 0, sizeof **out);
	return nout+nin;
}

void process_files(glite_jp_context_t ctx, const char *job, glite_jp_attrval_t** out, int* nout, const char* attr, const glite_jpps_fplug_data_t* plugin, const char* class, const char* uri, const char **opened_class, const char **opened_file, void **op_handle, void **of_handle){
	void *ph, *beh; 
	char** names = NULL;
        int nnames;
	int n;
	glite_jp_error_t	*keep_err = NULL;

	nnames = glite_jppsbe_get_names(ctx, job, class, &names);

        for (n = 0; n < nnames; n++){
		int plugin_ok = 0;
		if (*opened_class && !strcmp(*opened_class, class) && ((!*opened_file && !names[n]) || !strcmp(*opened_file, names[n]))){
			ph = *op_handle;
			beh = *of_handle;
			plugin_ok = 1;
		}
		else{
			if (*opened_class){
				free(*opened_class);
				*opened_class = NULL;
				free(*opened_file);
                                *opened_file = NULL;
				plugin->ops.close(plugin->fpctx, *op_handle);
				*op_handle = NULL;
				glite_jppsbe_close_file(ctx, *of_handle);
				*of_handle = NULL;
			}
			if (! glite_jppsbe_open_file(ctx,job,class, names[n], O_RDONLY, &beh) 
			&& !plugin->ops.open(plugin->fpctx,beh,uri,&ph)){
				plugin_ok = 1;
				*opened_class = strdup(class);
				if (names[n])
					*opened_file = strdup(names[n]);
				else
					*opened_file = NULL;
			        *op_handle = (void*)ph;
			        *of_handle = (void*)beh;
				printf("opening plugin %i at class %s, file %s\n", *op_handle, class, names[n]);
			}
		}
		if (plugin_ok){
		       	glite_jp_attrval_t* myattr;
        	        // XXX: ignore errors
                	if (!plugin->ops.attr(plugin->fpctx,ph,attr,&myattr) && myattr) {
                		int k;
	                       	for (k=0; myattr[k].name; k++) {
        	               		myattr[k].origin = GLITE_JP_ATTR_ORIG_FILE;
	               	                if (!myattr[k].origin_detail) 
						trio_asprintf(&myattr[k].origin_detail,"%s %s", uri, names[n] ? names[n] : "");
        	               	}
	                        *nout = merge_attrvals(out,*nout,myattr);
        	       	        free(myattr);
			}
			keep_err = ctx->error; ctx->error = NULL;
			if (keep_err) { ctx->error = keep_err; keep_err = NULL; }
               	}
		keep_err = ctx->error; ctx->error = NULL;
		if (keep_err) { ctx->error = keep_err; keep_err = NULL; }
	}
}

int glite_jpps_get_attrs(glite_jp_context_t ctx,const char *job,char **attr,int nattr,glite_jp_attrval_t **attrs_out)
{
	glite_jp_attrval_t	*meta = NULL,*out = NULL,*tag_out = NULL;
        char const	**other = NULL;
	int	i,j,nmeta,nother,err = 0,nout = 0;

	nmeta = nother = 0;
	glite_jp_clear_error(ctx);

/* sort the queried attributes to backend metadata and others -- retrived by plugins 
 * XXX: assumes unique values for metadata.
 */

	for (i=0; i<nattr; i++) {
		if (glite_jppsbe_is_metadata(ctx,attr[i])) {
			meta = realloc(meta,(nmeta+2) * sizeof *meta);
			memset(meta+nmeta,0,2 * sizeof *meta);
			meta[nmeta].name = strdup(attr[i]);
			nmeta++;
		}
		else {
			other = realloc(other,(nother+2) * sizeof *other);
			other[nother++] = attr[i]; /* XXX: not strdupped */
			other[nother] = NULL;
		}
	}

/* retrieve the metadata */
	if (meta && (err = glite_jppsbe_get_job_metadata(ctx,job,meta))) goto cleanup;

	if (!known_namespaces) scan_namespaces(ctx);

/* loop over the attributes */
	int k, l;
	for (i = 0; i < nother; i++){
		if (! glite_jppsbe_read_tag(ctx, job, other[i], &tag_out)) {
			nout = merge_attrvals(&out, nout, tag_out);
			free(tag_out); tag_out = NULL;
		}
		char* attr_namespace = glite_jpps_get_namespace(other[i]);
		for (j = 0; known_namespaces && known_namespaces[j].namespace; j++) {
			if (strcmp(attr_namespace, known_namespaces[j].namespace) == 0){
				for (k = 0; known_namespaces[j].plugins[k]; k++)
					for (l = 0; known_namespaces[j].plugins[k]->classes[l]; l++)
						process_files(ctx, job, &out, &nout, other[i], known_namespaces[j].plugins[k]
							, known_namespaces[j].plugins[k]->classes[l]
							, known_namespaces[j].plugins[k]->uris[l]
							, &known_namespaces[j].opened_classes[k]
							, &known_namespaces[j].opened_files[k]
							, &known_namespaces[j].plugin_handles[k]
							, &known_namespaces[j].file_handles[k]);
				break;
			}
		}
		free(attr_namespace);
	}

/* close plugins */
	for (i = 0; known_namespaces && known_namespaces[i].namespace; i++)
                for (j = 0; known_namespaces[i].plugins[j]; j++)
			if (known_namespaces[i].opened_classes[j]){
				known_namespaces[i].plugins[j]->ops.close(known_namespaces[i].plugins[j]->fpctx
					, known_namespaces[i].plugin_handles[j]);
				printf("closing plugin %i at class %s, file %s\n", known_namespaces[i].plugin_handles[j], known_namespaces[i].opened_classes[j], known_namespaces[i].opened_files[j]);
				glite_jppsbe_close_file(ctx, known_namespaces[i].file_handles[j]);
				known_namespaces[i].opened_classes[j] = NULL;
                	        known_namespaces[i].opened_files[j] = NULL;
				known_namespaces[i].plugin_handles[j] = NULL;
				known_namespaces[i].file_handles[j] = NULL;
			}

	nout = merge_attrvals(&out,nout,meta);

	free(meta); meta = NULL;

	for (i = 0; i < nout; i++)
                printf("%s\n", out[i].value);

	if (nout) {
		*attrs_out = out;
		err = 0;
	}
	else {
		glite_jp_error_t 	e;
		e.code = ENOENT;
		e.source = __FUNCTION__;
		e.desc = "no attributes found";
		err = glite_jp_stack_error(ctx,&e);
	}

cleanup:
	if (meta) for (i=0; i<nmeta; i++) glite_jp_attrval_free(meta+i,0);
	free(meta);

	free(other);

	return err;
}

