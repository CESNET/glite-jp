#include <stdio.h>
#include <fcntl.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "feed.h"

#include "jpps_H.h"
/* #include "JobProvenancePS.nsmap" */
#include "jpps_.nsmap" 

#include "jptype_map.h"

#include "file_plugin.h"
#include "builtin_plugins.h"

static struct jptype__GenericJPFaultType *jp2s_error(struct soap *soap,
		const glite_jp_error_t *err)
{
	struct jptype__GenericJPFaultType *ret = NULL;
	if (err) {
		ret = soap_malloc(soap,sizeof *ret);
		memset(ret,0,sizeof *ret);
		ret->code = err->code;
		ret->source = soap_strdup(soap,err->source);
		ret->text = soap_strdup(soap,strerror(err->code));
		ret->description = soap_strdup(soap,err->desc);
		ret->reason = jp2s_error(soap,err->reason);
	}
	return ret;
}

static void err2fault(const glite_jp_context_t ctx,struct soap *soap)
{
	char	*et;
	struct SOAP_ENV__Detail	*detail = soap_malloc(soap,sizeof *detail);
	struct _GenericJPFault *f = soap_malloc(soap,sizeof *f);


	f->jptype__GenericJPFault = jp2s_error(soap,ctx->error);

	detail->__type = SOAP_TYPE__GenericJPFault;
#if GSOAP_VERSION >= 20700
	detail->fault = f;
#else
	detail->value = f;
#endif
	detail->__any = NULL;

	soap_receiver_fault(soap,"Oh, shit!",NULL);
	if (soap->version == 2) soap->fault->SOAP_ENV__Detail = detail;
	else soap->fault->detail = detail;
}

/* deprecated 
static glite_jp_fileclass_t s2jp_fileclass(enum jptype__UploadClass class)
{
	switch (class) {
		case INPUT_SANDBOX: return GLITE_JP_FILECLASS_INPUT;
		case OUTPUT_SANDBOX: return GLITE_JP_FILECLASS_OUTPUT;
		case JOB_LOG: return GLITE_JP_FILECLASS_LBLOG;
		default: return GLITE_JP_FILECLASS_UNDEF;
	}
}
*/

static void s2jp_tag(const struct jptype__TagValue *stag,glite_jp_tagval_t *jptag)
{
	memset(jptag,0,sizeof *jptag);
	jptag->name = strdup(stag->name);
	jptag->sequence = stag->sequence ? *stag->sequence : 0;
	jptag->timestamp = stag->timestamp ? *stag->timestamp : 0;
	if (stag->stringValue) jptag->value = strdup(stag->stringValue);
	else if (stag->blobValue) {
		jptag->binary = 1;
		jptag->size = stag->blobValue->__size;
		jptag->value = (char *) stag->blobValue->__ptr;
	}
}

#define CONTEXT_FROM_SOAP(soap,ctx) glite_jp_context_t	ctx = (glite_jp_context_t) ((soap)->user)

SOAP_FMAC5 int SOAP_FMAC6 jpsrv__RegisterJob(
		struct soap *soap,
		char *job,
		struct jpsrv__RegisterJobResponse *response)
{
	CONTEXT_FROM_SOAP(soap,ctx);
	char	*owner = glite_jp_peer_name(ctx);
	glite_jp_attrval_t owner_val[2];

	if (glite_jppsbe_register_job(ctx,job,owner)) {
		err2fault(ctx,soap);
		free(owner);
		return SOAP_FAULT;
	}

	owner_val[0].attr.type = GLITE_JP_ATTR_OWNER;
	owner_val[0].value.s = owner;
	owner_val[1].attr.type = GLITE_JP_ATTR_UNDEF;

/* XXX: errrors should be ingored but not silently */
	glite_jpps_match_attr(ctx,job,owner_val); 
	free(owner);

	return SOAP_OK;
}


SOAP_FMAC5 int SOAP_FMAC6 jpsrv__StartUpload(
		struct soap *soap,
		char *job,
		char *class,
		char *name,
		time_t commit_before,
		char *content_type,
		struct jpsrv__StartUploadResponse *response)
{
	CONTEXT_FROM_SOAP(soap,ctx);
	char	*destination;

	if (glite_jppsbe_start_upload(ctx,job,class,name,content_type,&destination,&commit_before)) {
		err2fault(ctx,soap);
		return SOAP_FAULT;
	}

	response->destination = soap_strdup(soap,destination);
	free(destination);
	response->commitBefore = commit_before;

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 jpsrv__CommitUpload(
		struct soap *soap,
		char *destination,
		struct jpsrv__CommitUploadResponse *response)
{
	CONTEXT_FROM_SOAP(soap,ctx);
	char	*job,*class,*name;

	job = class = name = NULL;
	
	if (glite_jppsbe_commit_upload(ctx,destination)) {
		err2fault(ctx,soap);
		return SOAP_FAULT;
	}

	/* XXX: should not fail when commit_upload was OK */
	glite_jppsbe_destination_info(ctx,destination,&job,&class,&name);

	/* XXX: ignore errors but don't fail silenty */
	glite_jpps_match_file(ctx,job,class,name);

	free(job); free(class); free(name);

	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 jpsrv__RecordTag(
		struct soap *soap,
		char *job,
		struct jptype__TagValue *tag,
		struct jpsrv__RecordTagResponse *response)
{
	CONTEXT_FROM_SOAP(soap,ctx);
	void	*file_be,*file_p;
	glite_jpps_fplug_data_t	pd;

	glite_jp_tagval_t	mytag;

	file_be = file_p = NULL;

	/* XXX: we assume that TAGS plugin handles just one uri/class */
	if (glite_jpps_fplug_lookup(ctx,GLITE_JP_FILETYPE_TAGS,&pd)
		|| glite_jppsbe_open_file(ctx,job,pd.classes[0],NULL,
						O_WRONLY|O_CREAT,&file_be)
	) {
		err2fault(ctx,soap);
		return SOAP_FAULT;
	}

	s2jp_tag(tag,&mytag);

	if (pd.ops.open(pd.fpctx,file_be,&file_p)
		|| pd.ops.generic(pd.fpctx,file_p,GLITE_JP_FPLUG_TAGS_APPEND,&mytag))
	{
		err2fault(ctx,soap);
		if (file_p) pd.ops.close(pd.fpctx,file_p);
		glite_jppsbe_close_file(ctx,file_be);
		return SOAP_FAULT;
	}

	if (pd.ops.close(pd.fpctx,file_p)
		|| glite_jppsbe_close_file(ctx,file_be))
	{
		err2fault(ctx,soap);
		return SOAP_FAULT;
	}

	/* XXX: ignore errors but don't fail silenty */
	glite_jpps_match_tag(ctx,job,&mytag);

	return SOAP_OK;
}

static void s2jp_attr(const struct jptype__Attribute *in,glite_jp_attr_t *out)
{
	switch (in->type) {
		case OWNER: out->type = GLITE_JP_ATTR_OWNER; break;
		case TIME: out->type = GLITE_JP_ATTR_TIME;
			   out->name = strdup(in->name);
			   break;
		case TAG: out->type = GLITE_JP_ATTR_TAG;
			  out->name = strdup(in->name);
			  break;
		default: break;
	}
}

static void s2jp_queryval(
		const char *in,
		glite_jp_attrtype_t type,
		union _glite_jp_query_rec_val *out)
{
	switch (type) {
		case GLITE_JP_ATTR_OWNER:
		case GLITE_JP_ATTR_TAG:
			out->s = strdup(in);
			break;
		case GLITE_JP_ATTR_TIME:
			out->time.tv_sec = atoi(in);
			break;
	}
}

static void s2jp_query(const struct jptype__PrimaryQueryElement *in, glite_jp_query_rec_t *out)
{
	s2jp_attr(in->attr,&out->attr);

	switch (in->op) {
		case EQUAL: out->op = GLITE_JP_QUERYOP_EQUAL; break;
		case UNEQUAL: out->op = GLITE_JP_QUERYOP_UNEQUAL; break;
		case LESS: out->op = GLITE_JP_QUERYOP_LESS; break;
		case GREATER: out->op = GLITE_JP_QUERYOP_GREATER; break;
		case WITHIN:
			out->op = GLITE_JP_QUERYOP_WITHIN;
			s2jp_queryval(in->value2,out->attr.type,&out->value2);
			break;
	}

	s2jp_queryval(in->value,out->attr.type,&out->value);
}

SOAP_FMAC5 int SOAP_FMAC6 jpsrv__FeedIndex(
		struct soap *soap,
		char *destination,
		struct jptype__Attributes *attributes,
		struct jptype__PrimaryQuery *query,
		enum xsd__boolean history,
		enum xsd__boolean continuous,
	       	struct jpsrv__FeedIndexResponse *response)
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

	glite_jp_attr_t	*attrs = calloc(attributes->__sizeitem+1,sizeof *attrs);
	glite_jp_query_rec_t	*qry = calloc(query->__sizeitem+1,sizeof *qry);
	int	i;

	glite_jp_clear_error(ctx);

	for (i = 0; i<attributes->__sizeitem; i++) s2jp_attr(attributes->item[i],attrs+i);
	for (i = 0; i<query->__sizeitem; i++) s2jp_query(query->item[i],qry+i);

	if (history) {
		if (glite_jpps_run_feed(ctx,destination,attrs,qry,&feed_id)) {
			err2fault(ctx,soap);
			ret = SOAP_FAULT;
			goto cleanup;
		}
	}

	if (continuous) {
		if (glite_jpps_register_feed(ctx,destination,attrs,qry,&feed_id,&expires)) {
			err2fault(ctx,soap);
			ret = SOAP_FAULT;
			goto cleanup;
		}
	}

	if (!history && !continuous) {
		glite_jp_error_t	err;
		err.code = EINVAL;
		err.source = __FUNCTION__;
		err.desc = "at least one of <history> and <continous> must be true";
		glite_jp_stack_error(ctx,&err);
		err2fault(ctx,soap);
		ret = SOAP_FAULT;
		goto cleanup;
	}

	response->expires = expires;
	response->feedId = soap_strdup(soap,feed_id);

cleanup:
	free(feed_id);
	for (i=0; attrs[i].type; i++) free(attrs[i].name);
	free(attrs);
	for (i=0; qry[i].attr.type; i++) glite_jp_free_query_rec(qry+i);
	free(qry);

	return ret;
}

SOAP_FMAC5 int SOAP_FMAC6 jpsrv__FeedIndexRefresh(
		struct soap *soap,
		char *feed_id,
		struct jpsrv__FeedIndexRefreshResponse *response)
{
	fprintf(stderr,"%s: not implemented\n",__FUNCTION__);
	abort();
}

SOAP_FMAC5 int SOAP_FMAC6 jpsrv__GetJob(
		struct soap *soap,
		char *job,
		struct jpsrv__GetJobResponse *response)
{
	CONTEXT_FROM_SOAP(soap,ctx);
	char	*url;

	int	i,n;
	glite_jp_error_t	err;
	void	**pd;
	struct jptype__Files	*files;
	struct jptype__File 	**f = NULL;

	files = response->files = soap_malloc(soap,sizeof *response->files);
	files->__sizefile = 0;

	for (pd = ctx->plugins; *pd; pd++) {
		glite_jpps_fplug_data_t	*plugin = *pd;

		for (i=0; plugin->uris[i]; i++) {
			glite_jp_clear_error(ctx);
			switch (glite_jppsbe_get_job_url(ctx,job,plugin->classes[i],NULL,&url)) {
				case 0: n = files->__sizefile++;
					f = realloc(f,files->__sizefile * sizeof *f);
					f[n] = soap_malloc(soap, sizeof **f);
					f[n]->class_ = soap_strdup(soap,plugin->uris[i]);
					f[n]->name = NULL;
					f[n]->url = soap_strdup(soap,url);
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

	if (!files->__sizefile) {
		glite_jp_clear_error(ctx);
		err.code = ENOENT;
		err.source = __FUNCTION__;
		err.desc = "No file found for this job";
		glite_jp_stack_error(ctx,&err);
		err2fault(ctx,soap);
		glite_jp_clear_error(ctx);
		return SOAP_FAULT;
	}

	files->file = soap_malloc(soap,files->__sizefile * sizeof *f);
	memcpy(files->file,f,files->__sizefile * sizeof *f);

	return SOAP_OK;
}

