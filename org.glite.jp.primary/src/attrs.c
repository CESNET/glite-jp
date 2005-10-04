#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "glite/jp/types.h"
#include "glite/jp/attr.h"

#include "feed.h"
#include "backend.h"
#include "attrs.h"
#include "file_plugin.h"
#include "builtin_plugins.h"

static struct {
	char	*class,*uri;
	glite_jpps_fplug_data_t	**plugins;
	int	nplugins;
} *known_classes;

static int tags_index;


static void scan_classes(glite_jp_context_t ctx)
{
	int	i,j,k;
	glite_jpps_fplug_data_t	*pd;

	if (!ctx->plugins) return;
	for (i=0; ctx->plugins[i]; i++) {
		pd = ctx->plugins[i];

		for (j=0; pd->classes[j]; j++) {
			for (k=0; known_classes && known_classes[k].class
					&& strcmp(pd->classes[j],known_classes[k].class);
				k++);
			if (known_classes && known_classes[k].class) {
				known_classes[k].plugins = realloc(known_classes[k].plugins,
						(known_classes[k].nplugins + 2) * sizeof(glite_jpps_fplug_data_t *));
				known_classes[k].plugins[known_classes[k].nplugins++] = pd;
				known_classes[k].plugins[known_classes[k].nplugins] = NULL;
			}
			else {
				known_classes = realloc(known_classes,(k+2) * sizeof *known_classes);
				known_classes[k].class = pd->classes[j];
				known_classes[k].uri = pd->uris[j];
				known_classes[k].plugins = malloc(2 * sizeof(glite_jpps_fplug_data_t *));
				known_classes[k].plugins[0] = pd;
				known_classes[k].plugins[1] = NULL;
				known_classes[k].nplugins = 1;
				memset(known_classes+k+1,0,sizeof *known_classes);
				if (!strcmp(known_classes[k].uri,GLITE_JP_FILETYPE_TAGS)) tags_index = k;
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
	return nout+nin;
}

glite_jpps_get_attrs(glite_jp_context_t ctx,const char *job,char **attr,int nattr,glite_jp_attrval_t **attrs_out)
{
	glite_jp_attrval_t	*meta = NULL,*out = NULL;
        char const	**other = NULL;
	int	i,j,nmeta,nother,err = 0,nout = 0;

	struct { int 	class_idx;
		 char	*name;
	} *files = NULL;
	int	nfiles = 0;

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

	if (!known_classes) scan_classes(ctx);

/* build a list of available files for this job */
	files = malloc(sizeof *files);
	files->class_idx = tags_index;
	files->name = NULL;
	nfiles = 1;

	for (i=0; known_classes[i].class; i++) {
		char	**names = NULL;
		int	nnames = 
			glite_jppsbe_get_names(ctx,job,known_classes[i].class,&names);
		if (nnames < 0) continue; /* XXX: error ignored */

		if (nnames > 0) {
			files = realloc(files,nfiles+nnames+1 * sizeof *files);
			for (j=0; j<nnames; j++) {
				files[nfiles].class_idx = i;
				files[nfiles++].name = names[j];
			}
		}
		free(names);
	}

/* loop over the files */
	for (i=0; i<nfiles; i++) {
		void	*beh;
		int	ci;

		/* XXX: ignore error */
		if (!glite_jppsbe_open_file(ctx,job,
			known_classes[ci = files[i].class_idx].class,
			files[i].name,O_RDONLY,&beh))
		{
			for (j=0; j<known_classes[ci].nplugins; j++) {
				void	*ph;

				glite_jpps_fplug_data_t	*p = 
					known_classes[ci].plugins[j];
				/* XXX: ignore error */
				if (!p->ops.open(p->fpctx,beh,known_classes[ci].uri,&ph)) {

					for (j=0; j<nother; j++) {
						glite_jp_attrval_t	*myattr;
						/* XXX: ignore errors */
						if (!p->ops.attr(p->fpctx,ph,other[j],&myattr)) {
							int	k;
							for (k=0; myattr[k].name; k++) {
								myattr[k].origin = GLITE_JP_ATTR_ORIG_FILE;
								trio_asprintf(&myattr[k].origin_detail,"%s %s",
										known_classes[ci].uri,
										files[i].name ? files[i].name : "");
							}
							nout = merge_attrvals(&out,nout,myattr);
							free(myattr);
						}

					}
					p->ops.close(p->fpctx,ph);
				}
			}

			glite_jppsbe_close_file(ctx,beh);
		}
	}

	nout = merge_attrvals(&out,nout,meta);
	free(meta); meta = NULL;

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

	if (files) for (i=0; i<nfiles; i++) free(files[i].name);
	free(files);
	
	return err;
}
