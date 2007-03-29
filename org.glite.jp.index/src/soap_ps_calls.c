#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "soap_version.h"
#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/security/glite_gsplugin.h"
#include "glite/security/glite_gscompat.h"

#include "jpps_H.h"
#include "jpps_.nsmap"

#include "conf.h"
#include "db_ops.h"
#include "ws_ps_typeref.h"
#include "context.h"

#include "stdsoap2.h"


/*------------------*/
/* Helper functions */
/*------------------*/

int debug = 0;
#define dprintf(FMT, ARGS...) if (debug) fprintf(stderr, FMT, ##ARGS)
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


// call PS FeedIndex for a given destination
int MyFeedIndex(glite_jpis_context_t ctx, glite_jp_is_conf *conf, long int uniqueid, char *dest)
{
	struct _jpelem__FeedIndex		in;
	struct _jpelem__FeedIndexResponse 	out;
//	struct jptype__primaryQuery     	query;
//	struct jptype__stringOrBlob		value;
//	struct xsd__base64Binary		blob;
	int 					i, dest_index;
	struct soap             		*soap = soap_new();
	glite_gsplugin_Context			plugin_ctx;
	glite_jp_error_t err;
	char *src, *desc = NULL;

	lprintf("(%ld) for %s called\n", uniqueid, dest);
	debug = conf->debug;

	glite_gsplugin_init_context(&plugin_ctx);
	if (ctx->conf->server_key) plugin_ctx->key_filename = strdup(ctx->conf->server_key);
	if (ctx->conf->server_cert) plugin_ctx->cert_filename = strdup(ctx->conf->server_cert);
	
	soap_init(soap);
        soap_set_namespaces(soap,jpps__namespaces);
	soap_set_omode(soap, SOAP_IO_BUFFER);   // set buffered response
                                                // buffer set to SOAP_BUFLEN (default = 8k)	
	soap_register_plugin_arg(soap,glite_gsplugin,plugin_ctx);

	memset(&in, 0, sizeof(in));
	memset(&err, 0, sizeof(err));

	for (i=0; conf->attrs[i]; i++) ;
	in.__sizeattributes = i;
	in.attributes = conf->attrs;

	if ((dest_index = find_dest_index(conf, uniqueid)) < 0) goto err;

	for (i=0; conf->feeds[dest_index]->query[i]; i++);
	GLITE_SECURITY_GSOAP_LIST_CREATE(soap, &in, conditions, struct jptype__primaryQuery, i);

	for (i=0; conf->feeds[dest_index]->query[i]; i++) {
		if (glite_jpis_QueryCondToSoap(soap, conf->feeds[dest_index]->query[i], 
				GLITE_SECURITY_GSOAP_LIST_GET(in.conditions, i)) != SOAP_OK) {
			err.code = EINVAL;
			err.desc = "error during conds conversion";
			asprintf(&src, "%s/%s():%d", __FILE__, __FUNCTION__, __LINE__);
			fprintf(stderr, "%s\n", src);
			goto err;
		}
	}

	in.history = conf->feeds[dest_index]->history;
	in.continuous = conf->feeds[dest_index]->continuous;
	in.destination = ctx->hname;
	lprintf("(%ld) destination IS: %s\n", uniqueid, ctx->hname);

	if (check_fault(soap,soap_call___jpsrv__FeedIndex(soap,dest,"", &in, &out)) != 0) {
		fprintf(stderr, "\n");
		glite_jpis_unlockFeed(ctx, uniqueid);
		err.code = EIO;
		asprintf(&desc, "soap_call___jpsrv__FeedIndex() returned error %d", soap->error);
		err.desc = desc;
		asprintf(&src, "%s/%s():%d", __FILE__, __FUNCTION__, __LINE__);
		fprintf(stderr, "%s\n", err.desc);
		goto err;
	}
	else {
		lprintf("(%ld) FeedId: %s\n", uniqueid, out.feedId);
		lprintf("(%ld) Expires: %s", uniqueid, ctime(&out.feedExpires));
		glite_jpis_initFeed(ctx, uniqueid, out.feedId, out.feedExpires);
		glite_jpis_unlockFeed(ctx, uniqueid);
	}

	soap_end(soap);
	soap_done(soap);

	return 0;

err:
	err.source = src;
	glite_jp_stack_error(ctx->jpctx, &err);
	free(src);
	free(desc);
	soap_end(soap);
	soap_done(soap);

	return err.code;
}

