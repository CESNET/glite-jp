#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>

#undef SOAP_FMAC1
#define SOAP_FMAC1	static

#include <stdsoap2.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/attr.h"
#include "glite/jp/known_attr.h"

#include "feed.h"
#include "attrs.h"

#include "jptype_map.h"
#include "glite/security/glite_gscompat.h"

#include "file_plugin.h"
#include "builtin_plugins.h"

/* the same as ServerLib.c but without WITH_NOGLOBAL which breaks the soap_env_ctx trick */
#define SOAP_FMAC3 static
#include "jpps_C.c"
#include "jpps_Server.c"

#include "jpps_.nsmap"

#include "soap_env_ctx.h"
#include "soap_env_ctx.c"

#include "glite/jp/ws_fault.c"
#include "soap_util.c"

#define err2fault(CTX, SOAP) glite_jp_server_err2fault((CTX), (SOAP));

#define CONTEXT_FROM_SOAP(soap,ctx) glite_jp_context_t	ctx = (glite_jp_context_t) ((soap)->user)

#define SIZE_TO_COMPRESS 1024

int glite_jpps_srv_init(glite_jp_context_t ctx)
{
	glite_jp_soap_env_ctx = &my_soap_env_ctx;
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__RegisterJob(
		struct soap *soap,
		struct _jpelem__RegisterJob *in,
		struct _jpelem__RegisterJobResponse *empty)
{
	CONTEXT_FROM_SOAP(soap,ctx);
	glite_jp_attrval_t owner_val[2];

	printf("%s %s %s\n",__FUNCTION__,in->job,in->owner);
	if (glite_jpps_authz(ctx,SOAP_TYPE___jpsrv__RegisterJob,in->job,in->owner) ||
		glite_jppsbe_register_job(ctx,in->job,in->owner))
	{
		err2fault(ctx,soap);
		return SOAP_FAULT;
	}

	memset(owner_val, 0, 2 * sizeof(glite_jp_attrval_t));
	owner_val[0].name = GLITE_JP_ATTR_OWNER;
	owner_val[0].value = in->owner;
	owner_val[0].origin = GLITE_JP_ATTR_ORIG_SYSTEM;
	owner_val[0].timestamp = time(NULL);
	owner_val[0].origin_detail = NULL;
	owner_val[1].name = NULL;

/* XXX: errrors should be ingored but not silently */
	glite_jpps_match_attr(ctx,in->job,owner_val); 

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__StartUpload(
		struct soap *soap,
		struct _jpelem__StartUpload *in,
		struct _jpelem__StartUploadResponse *out)
{
	CONTEXT_FROM_SOAP(soap,ctx);
	char	*destination;
	time_t	commit_before = in->commitBefore;
	glite_jp_error_t	err;
	glite_jpps_fplug_data_t	**pd = NULL;
	int	i;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);

	if (glite_jpps_authz(ctx,SOAP_TYPE___jpsrv__StartUpload,NULL,NULL)) {
		err2fault(ctx,soap);
		return SOAP_FAULT;
	}

	switch (glite_jpps_fplug_lookup(ctx,in->class_,&pd)) {
		case ENOENT:
			err.code = ENOENT;
			err.source = __FUNCTION__;
			err.desc = "unknown file class";
			glite_jp_stack_error(ctx,&err);
			err2fault(ctx,soap);
			return SOAP_FAULT;
		case 0: break;
		default:
			err2fault(ctx,soap);
			return SOAP_FAULT;
	}

	for (i=0; pd[0]->uris[i] && strcmp(pd[0]->uris[i],in->class_); i++);
	assert(pd[0]->uris[i]);

	if (glite_jppsbe_start_upload(ctx,in->job,pd[0]->classes[i],in->name,in->contentType,
				&destination,&commit_before))
	{
		err2fault(ctx,soap);
		free(pd);
		return SOAP_FAULT;
	}

	out->destination = soap_strdup(soap,destination);
	free(destination);
	out->commitBefore = commit_before;

	free(pd);
	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__CommitUpload(
		struct soap *soap,
		struct _jpelem__CommitUpload *in,
		struct _jpelem__CommitUploadResponse *out)
{
	CONTEXT_FROM_SOAP(soap,ctx);
	char	*job,*class,*name;

	job = class = name = NULL;
	
	if (glite_jpps_authz(ctx,SOAP_TYPE___jpsrv__CommitUpload,NULL,NULL) ||
		glite_jppsbe_commit_upload(ctx,in->destination))
	{
		err2fault(ctx,soap);
		return SOAP_FAULT;
	}

	/* XXX: should not fail when commit_upload was OK */
	assert(glite_jppsbe_destination_info(ctx,in->destination,&job,&class,&name) == 0);

	/* XXX: ignore errors but don't fail silenty */
	glite_jpps_match_file(ctx,job,class,name);

	// apply plugins to commited file
        glite_jpps_fplug_data_t *pd;
	int i, j;
	void *beh, *ph;
        if (ctx->plugins)
                for (i = 0; ctx->plugins[i]; i++) {
                        pd = ctx->plugins[i];
                        if (pd->classes)
                                for (j = 0; pd->classes[j]; j++)
					if (strcmp(class, pd->classes[j]) == 0){
						if (! glite_jppsbe_open_file(ctx,job,class, name, O_RDONLY, &beh)) {
                        				if (!pd->ops.open(pd->fpctx,beh,pd->uris[j],&ph)) {
                                				pd->ops.filecom(pd->fpctx, ph);
								pd->ops.close(pd->fpctx, ph);
                        				}
                        				glite_jppsbe_close_file(ctx,beh);
                				}

                                	}
                }

	char *fname;
	//XXX ignore error
	if (!glite_jppsbe_open_file(ctx,job,class, name, O_RDONLY, &beh)){
		struct stat fattr;
		glite_jppsbe_file_attrs(ctx, beh, &fattr);
		glite_jppsbe_close_file(ctx, beh);
		if (fattr.st_size > SIZE_TO_COMPRESS)
			glite_jppsbe_compress_and_remove_file(ctx,job,class, name);
	}

	free(job); free(class); free(name);

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__RecordTag(
		struct soap *soap,
		struct _jpelem__RecordTag *in,
		struct _jpelem__RecordTagResponse *out)
{
	CONTEXT_FROM_SOAP(soap,ctx);
	void	*file_be,*file_p;
	glite_jp_attrval_t	attr[2], meta[2];

	file_be = file_p = NULL;

	memset(attr, 0, 2 * sizeof(glite_jp_attrval_t));
	memset(meta,0,sizeof meta);
        meta[0].name = strdup(GLITE_JP_ATTR_OWNER);

	if (glite_jppsbe_get_job_metadata(ctx,in->jobid,meta)) {
		goto err;
	}
	
	if (glite_jpps_authz(ctx,SOAP_TYPE___jpsrv__RecordTag,in->jobid,meta[0].value)) {
		goto err;
	}

	attr[0].name = in->tag->name;
	if (GSOAP_ISSTRING(in->tag->value)) {
		attr[0].value = GSOAP_STRING(in->tag->value);
		attr[0].binary = 0;
	}
	else {
		attr[0].value = GSOAP_BLOB(in->tag->value)->__ptr;
		attr[0].size = GSOAP_BLOB(in->tag->value)->__size;
		attr[0].binary = 1;
	}
	attr[0].origin = GLITE_JP_ATTR_ORIG_USER;
	attr[0].timestamp = time(NULL);
	attr[0].origin_detail = NULL; 	/* XXX */
	attr[1].name = NULL;

        /*if (glite_jppsbe_open_file(ctx,in->jobid,"tags",NULL,
                                                O_RDWR|O_CREAT,&file_be)
                        // XXX: tags need reading to check magic number
        ) {
                err2fault(ctx,soap);
                return SOAP_FAULT;
        }

	if (glite_jppsbe_close_file(ctx,file_be))
	{
		err2fault(ctx,soap);
		return SOAP_FAULT;
	}*/
	glite_jppsbe_append_tag(ctx,in->jobid,attr);

        /*if (tag_append(ctx,file_be,attr))
	{
                err2fault(ctx,soap);
                return SOAP_FAULT;
        }*/

	/* XXX: ignore errors but don't fail silenty */
	glite_jpps_match_attr(ctx,in->jobid,attr);

	return SOAP_OK;
err:
	glite_jp_attrval_free(meta,0);
	err2fault(ctx,soap);
	return SOAP_FAULT;
}

static void s2jp_qval(const struct jptype__stringOrBlob *in, char **value, int *binary, size_t *size)
{
	if (GSOAP_ISSTRING(in)) {
		*value = GSOAP_STRING(in);
		*binary = 0;
		*size = 0;
	}
	else {
		assert(GSOAP_BLOB(in));	/* XXX: should report error instead */
		*value = GSOAP_BLOB(in)->__ptr;
		*binary = 1;
		*size = GSOAP_BLOB(in)->__size;
	}
}

static void s2jp_query(const struct jptype__primaryQuery *in, glite_jp_query_rec_t *out)
{
	int	b;

	out->attr = in->attr;

	s2jp_qval(in->value,&out->value,&out->binary,&out->size);
	switch (in->op) {
		case EQUAL: out->op = GLITE_JP_QUERYOP_EQUAL; break;
		case UNEQUAL: out->op = GLITE_JP_QUERYOP_UNEQUAL; break;
		case LESS: out->op = GLITE_JP_QUERYOP_LESS; break;
		case GREATER: out->op = GLITE_JP_QUERYOP_GREATER; break;
		case WITHIN:
			out->op = GLITE_JP_QUERYOP_WITHIN;
			s2jp_qval(in->value2,&out->value2,&b,&out->size2);
			assert(out->binary == b);	/* XXX: report error instead */

			break;
	}

	if (in->origin) switch (*in->origin) {
		case jptype__attrOrig__SYSTEM: out->origin = GLITE_JP_ATTR_ORIG_SYSTEM; break;
		case jptype__attrOrig__USER: out->origin = GLITE_JP_ATTR_ORIG_USER; break;
		case jptype__attrOrig__FILE_: out->origin = GLITE_JP_ATTR_ORIG_FILE; break;
	}
	else out->origin = GLITE_JP_ATTR_ORIG_ANY;
}


static int check_sane_feed(glite_jp_context_t ctx,struct _jpelem__FeedIndex *in)
{
	glite_jp_error_t	err;
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	err.code = EINVAL;

	if (!in->destination) {
		err.desc = "destination required";
		return glite_jp_stack_error(ctx,&err);
	}

	return 0;
}


SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__FeedIndex(
		struct soap *soap,
		struct _jpelem__FeedIndex *in,
		struct _jpelem__FeedIndexResponse *out)
{	

/* deferred processing: return feed_id to the index server first,
 * start feeding it afterwards -- not before the index server actually
 * knows feed_id and is ready to accept the feed.
 *
 * Has to be done within the same server slave, 
 * passed through the context */

	CONTEXT_FROM_SOAP(soap,ctx);
	char	*feed_id = NULL;
	time_t	expires = 0;
	int 	ret = SOAP_OK;

	char	const **attrs = calloc(in->__sizeattributes+1,sizeof *attrs);
	glite_jp_query_rec_t	*qry = calloc(in->__sizeconditions+1,sizeof *qry);
	int	i;

	glite_jp_clear_error(ctx);

	if (check_sane_feed(ctx,in)) {
		err2fault(ctx,soap);
		ret = SOAP_FAULT;
		goto cleanup;
	}

	memcpy(attrs,in->attributes,sizeof *attrs * in->__sizeattributes);
	for (i = 0; i<in->__sizeconditions; i++) s2jp_query(GLITE_SECURITY_GSOAP_LIST_GET(in->conditions, i),qry+i);

	if (in->history) {
		if (glite_jpps_run_feed(ctx,in->destination,attrs,qry,in->continuous,&feed_id)) {
			err2fault(ctx,soap);
			ret = SOAP_FAULT;
			goto cleanup;
		}
	}

	if (in->continuous) {
		if (glite_jpps_register_feed(ctx,in->destination,attrs,qry,&feed_id,&expires)) {
			err2fault(ctx,soap);
			ret = SOAP_FAULT;
			goto cleanup;
		}
	}

	if (!in->history && !in->continuous) {
		glite_jp_error_t	err;
		memset(&err,0,sizeof err);
		err.code = EINVAL;
		err.source = __FUNCTION__;
		err.desc = "at least one of <history> and <continous> must be true";
		glite_jp_stack_error(ctx,&err);
		err2fault(ctx,soap);
		ret = SOAP_FAULT;
		goto cleanup;
	}

	out->feedExpires = expires;
	out->feedId = soap_strdup(soap,feed_id);

cleanup:
	free(feed_id);
	free(attrs);
	free(qry);

	return ret;
}

SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__FeedIndexRefresh(
		struct soap *soap,
		struct _jpelem__FeedIndexRefresh *in,
		struct _jpelem__FeedIndexRefreshResponse *out)
{
	fprintf(stderr,"%s: not implemented\n",__FUNCTION__);
	abort();
}

SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__GetJobFiles(
		struct soap *soap,
		struct _jpelem__GetJobFiles *in,
		struct _jpelem__GetJobFilesResponse *out)
{
	CONTEXT_FROM_SOAP(soap,ctx);
	char	*url;

	int	i,n;
	glite_jp_error_t	err;
	void	**pd;
	struct jptype__jppsFile 	*f = NULL;
	glite_jp_attrval_t	meta[2];

	memset(&err,0,sizeof err);
	n = 0;

	memset(meta,0,sizeof meta);
        meta[0].name = strdup(GLITE_JP_ATTR_OWNER);

	if (glite_jppsbe_get_job_metadata(ctx,in->jobid,meta)) {
		goto err;
	}
	
	if (glite_jpps_authz(ctx,SOAP_TYPE___jpsrv__GetJobFiles,in->jobid,meta[0].value)) {
		goto err;
	}

	memset(meta,0,sizeof meta);
        meta[0].name = strdup(GLITE_JP_ATTR_OWNER);

	if (glite_jppsbe_get_job_metadata(ctx,in->jobid,meta)) {
		goto err;
	}
	
	if (glite_jpps_authz(ctx,SOAP_TYPE___jpsrv__GetJobFiles,in->jobid,meta[0].value)) {
		goto err;
	}

	for (pd = ctx->plugins; *pd; pd++) {
		glite_jpps_fplug_data_t	*plugin = *pd;

		for (i=0; plugin->uris[i]; i++) {
			glite_jp_clear_error(ctx);
			switch (glite_jppsbe_get_job_url(ctx,in->jobid,plugin->classes[i],NULL,&url)) {
				case 0:
					f = realloc(f,(n + 1) * sizeof *f);
					f[n].class_ = soap_strdup(soap,plugin->uris[i]);
#warning FIXME: file name required in WSDL
					f[n].name = NULL;
					f[n].url = soap_strdup(soap,url);
					n++;
					free(url);
					break;
				case ENOENT:
					break;
				default: 
					err.code = ctx->error->code;
					err.source = "jpsrv__GetJob()";
					err.desc = plugin->uris[i];
					glite_jp_stack_error(ctx,&err);
					err2fault(ctx,soap);
					glite_jp_clear_error(ctx);
					return SOAP_FAULT;
			}
		}
	}

	if (!n) {
		glite_jp_clear_error(ctx);
		err.code = ENOENT;
		err.source = __FUNCTION__;
		err.desc = "No file found for this job";
		glite_jp_stack_error(ctx,&err);
		err2fault(ctx,soap);
//		glite_jp_clear_error(ctx);
		return SOAP_FAULT;
	}

	GLITE_SECURITY_GSOAP_LIST_CREATE(soap, out, files, struct jptype__jppsFile, n);
	for (i = 0; i < n; i++) memcpy(GLITE_SECURITY_GSOAP_LIST_GET(out->files, i), &f[i], sizeof(f[i]));

	return SOAP_OK;
err:
	glite_jp_attrval_free(meta,0);
	err2fault(ctx,soap);
	return SOAP_FAULT;
}

SOAP_FMAC5 int SOAP_FMAC6 __jpsrv__GetJobAttributes(
		struct soap *soap,
		struct _jpelem__GetJobAttributes *in,
		struct _jpelem__GetJobAttributesResponse *out)
{
	glite_jp_attrval_t	*attr, meta[2];
	int	i,n;

	CONTEXT_FROM_SOAP(soap,ctx);

	memset(meta,0,sizeof meta);
        meta[0].name = strdup(GLITE_JP_ATTR_OWNER);

	if (glite_jppsbe_get_job_metadata(ctx,in->jobid,meta)) {
		goto err;
	}
	
	if (glite_jpps_authz(ctx,SOAP_TYPE___jpsrv__GetJobAttributes,in->jobid,meta[0].value)) {
		goto err;
	}

	if (glite_jpps_get_attrs(ctx,in->jobid,
			in->attributes,
			in->__sizeattributes,&attr)) {
		err2fault(ctx,soap);
		return SOAP_FAULT;
	}

	for (i=0; attr[i].name; i++);
	out->__sizeattrValues = jp2s_attrValues(soap,attr,&out->attrValues,1);

	return SOAP_OK;
err:
	glite_jp_attrval_free(meta,0);
	err2fault(ctx,soap);
	return SOAP_FAULT;
}
