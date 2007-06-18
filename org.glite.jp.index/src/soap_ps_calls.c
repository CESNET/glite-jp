#include <sys/time.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "soap_version.h"
#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/security/glite_gss.h"
#include "glite/security/glite_gsplugin.h"
#include "glite/security/glite_gscompat.h"

#include "jp_H.h"

#include "conf.h"
#include "db_ops.h"
#include "ws_ps_typeref.h"
#include "context.h"
#include "common.h"

#include "stdsoap2.h"


extern struct Namespace jp__namespaces[];


/*------------------*/
/* Helper functions */
/*------------------*/

#define dprintf(FMT, ARGS...) do {fprintf(stderr, "[%d] %s: ", getpid(), __FUNCTION__); fprintf(stderr, FMT, ##ARGS); } while(0);
#include "glite/jp/ws_fault.c"
#define check_fault(SOAP, ERR) glite_jp_clientCheckFault((SOAP), (ERR), NULL, 0)


/*----------------------*/
/* PS WSDL client calls */
/*----------------------*/

static int find_dest_index(glite_jp_is_conf *conf, long int uniqueid)
{
	int i;

	for (i=0; conf->feeds[i]; i++)
		if (conf->feeds[i]->uniqueid == uniqueid) return(i);

	return -1;
}


static int refresh_gsoap(glite_jpis_context_t ctx, struct soap *soap) {
	gss_cred_id_t		cred;
	edg_wll_GssStatus	gss_code;
	char			*et;
	// preventive very long timeout
	static const struct timeval to = {tv_sec: 7200, tv_usec: 0};
	glite_gsplugin_Context	plugin_ctx;

	if (edg_wll_gss_acquire_cred_gsi(ctx->conf->server_cert, ctx->conf->server_key, &cred, NULL, &gss_code) != 0) {
		edg_wll_gss_get_error(&gss_code,"",&et);
		glite_jpis_stack_error(ctx->jpctx, EINVAL, "can't refresh certificates (%s)", et);
		free(et);
		return EINVAL;
		//printf("[%d] %s: %s\n", getpid(), __FUNCTION__, err.desc);
	}

	plugin_ctx = glite_gsplugin_get_context(soap);
	glite_gsplugin_set_timeout(plugin_ctx, &to);
	glite_gsplugin_set_credential(plugin_ctx, cred);

	return 0;
}


// call PS FeedIndex for a given destination
int MyFeedIndex(struct soap *soap, glite_jpis_context_t ctx, long int uniqueid, const char *dest)
{
	struct _jpelem__FeedIndex		in;
	struct _jpelem__FeedIndexResponse 	out;
//	struct jptype__primaryQuery     	query;
//	struct jptype__stringOrBlob		value;
//	struct xsd__base64Binary		blob;
	int 					i, dest_index, status;
	glite_jp_is_conf *conf = ctx->conf;

	lprintf("(%ld) for %s called\n", uniqueid, dest);

	if (refresh_gsoap(ctx, soap) != 0)
		return glite_jpis_stack_error(ctx->jpctx, EINVAL, "can't refresh credentials");

	memset(&in, 0, sizeof(in));

	for (i=0; conf->attrs[i]; i++) ;
	in.__sizeattributes = i;
	in.attributes = conf->attrs;

	if ((dest_index = find_dest_index(conf, uniqueid)) < 0)
		return glite_jpis_stack_error(ctx->jpctx, EINVAL, "internal error (feed index %ld not found)", uniqueid);

	soap_begin(soap);
	for (i=0; conf->feeds[dest_index]->query[i].attr; i++);
	GLITE_SECURITY_GSOAP_LIST_CREATE(soap, &in, conditions, struct jptype__primaryQuery, i);

	for (i=0; conf->feeds[dest_index]->query[i].attr; i++) {
		if (glite_jpis_QueryCondToSoap(soap, &conf->feeds[dest_index]->query[i], 
				GLITE_SECURITY_GSOAP_LIST_GET(in.conditions, i)) != SOAP_OK) {
			soap_end(soap);
			return glite_jpis_stack_error(ctx->jpctx, EINVAL, "error during conds conversion");
		}
	}

	in.history = conf->feeds[dest_index]->history;
	in.continuous = conf->feeds[dest_index]->continuous;
	in.destination = ctx->hname;
	lprintf("(%ld) destination IS: '%s'\n", uniqueid, ctx->hname);

	if (check_fault(soap,soap_call___jpsrv__FeedIndex(soap,dest,"", &in, &out)) != 0) {
		fprintf(stderr, "\n");
		glite_jpis_unlockFeed(ctx, uniqueid);
		glite_jpis_stack_error(ctx->jpctx, EIO, "soap_call___jpsrv__FeedIndex() returned error %d", soap->error);
		soap_end(soap);
		return EIO;
	}
	else {
		status = (conf->feeds[dest_index]->history ? GLITE_JP_IS_STATE_HIST : 0) | (conf->feeds[dest_index]->continuous ? GLITE_JP_IS_STATE_CONT : 0);
		lprintf("(%ld) FeedId: %s\n", uniqueid, out.feedId);
		lprintf("(%ld) Expires: %s", uniqueid, ctime(&out.feedExpires));
		glite_jpis_initFeed(ctx, uniqueid, out.feedId, time(NULL) + (out.feedExpires - time(NULL)) / 2, status);
		glite_jpis_unlockFeed(ctx, uniqueid);
	}

	soap_end(soap);

	return 0;
}


int MyFeedRefresh(struct soap *soap, glite_jpis_context_t ctx, long int uniqueid, const char *dest, int status, const char *feedid)
{
	struct _jpelem__FeedIndexRefresh		in;
	struct _jpelem__FeedIndexRefreshResponse 	out;

	lprintf("(%ld) for %s called, status = %d\n", uniqueid, feedid, status);

	if (refresh_gsoap(ctx, soap) != 0)
		return glite_jpis_stack_error(ctx->jpctx, EINVAL, "can't refresh credentials");

	soap_begin(soap);
	memset(&in, 0, sizeof(in));
	in.feedId = soap_strdup(soap, feedid);
	if (check_fault(soap,soap_call___jpsrv__FeedIndexRefresh(soap,dest,"", &in, &out)) != 0) {
		fprintf(stderr, "\n");
		glite_jpis_unlockFeed(ctx, uniqueid);
		glite_jpis_stack_error(ctx->jpctx, EIO, "soap_call___jpsrv__FeedRefresh() returned error %d", soap->error);
		soap_end(soap);
		return EIO;
	}
	else {
		status &= (~GLITE_JP_IS_STATE_ERROR);
		lprintf("(%ld) FeedId: %s\n", uniqueid, feedid);
		lprintf("(%ld) Expires: %s", uniqueid, ctime(&out.feedExpires));
		glite_jpis_initFeed(ctx, uniqueid, feedid, time(NULL) + (out.feedExpires - time(NULL)) / 2, status);
		glite_jpis_unlockFeed(ctx, uniqueid);
	}

	soap_end(soap);
	return 0;
}


int __jpsrv__RegisterJob(struct soap* soap UNUSED, struct _jpelem__RegisterJob *jpelem__RegisterJob UNUSED, struct _jpelem__RegisterJobResponse *jpelem__RegisterJobResponse UNUSED) { return 0; }
int __jpsrv__StartUpload(struct soap* soap UNUSED, struct _jpelem__StartUpload *jpelem__StartUpload UNUSED, struct _jpelem__StartUploadResponse *jpelem__StartUploadResponse UNUSED) { return 0; }
int __jpsrv__CommitUpload(struct soap* soap UNUSED, struct _jpelem__CommitUpload *jpelem__CommitUpload UNUSED, struct _jpelem__CommitUploadResponse *jpelem__CommitUploadResponse UNUSED) { return 0; }
int __jpsrv__RecordTag(struct soap* soap UNUSED, struct _jpelem__RecordTag *jpelem__RecordTag UNUSED, struct _jpelem__RecordTagResponse *jpelem__RecordTagResponse UNUSED) { return 0; }
int __jpsrv__FeedIndex(struct soap* soap UNUSED, struct _jpelem__FeedIndex *jpelem__FeedIndex UNUSED, struct _jpelem__FeedIndexResponse *jpelem__FeedIndexResponse UNUSED) { return 0; }
int __jpsrv__FeedIndexRefresh(struct soap* soap UNUSED, struct _jpelem__FeedIndexRefresh *jpelem__FeedIndexRefresh UNUSED, struct _jpelem__FeedIndexRefreshResponse *jpelem__FeedIndexRefreshResponse UNUSED) { return 0; }
int __jpsrv__GetJobFiles(struct soap* soap UNUSED, struct _jpelem__GetJobFiles *jpelem__GetJobFiles UNUSED, struct _jpelem__GetJobFilesResponse *jpelem__GetJobFilesResponse UNUSED) { return 0; }
int __jpsrv__GetJobAttributes(struct soap* soap UNUSED, struct _jpelem__GetJobAttributes *jpelem__GetJobAttributes UNUSED, struct _jpelem__GetJobAttributesResponse *jpelem__GetJobAttributesResponse UNUSED) { return 0; }
