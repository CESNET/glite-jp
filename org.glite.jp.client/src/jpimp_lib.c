#include <time.h>
#include <errno.h>

#include "glite/lb/lb_maildir.h"

#include "jp_client.h"
#include "jpimporter.h"
#include "jpcl_ctx_int.h"

int glite_jpimporter_upload_files(
	glite_jpcl_context_t	ctx,
	const char			   *jobid,
	const char			  **files,
	const char			   *proxy)
{
	char	   *msg,
			   *file;

	if ( !files || !files[0] ) {
		glite_jpcl_SetError(ctx, EINVAL, "No files given");
		return -1;
	}
	if ( !jobid ) {
		glite_jpcl_SetError(ctx, EINVAL, "No jobid given");
		return -1;
	}
	/* TODO: get the user proxy if it is not specified and
	 * find the file of its location.
	 */

	if ( edg_wll_MaildirInit(ctx->lbmd_dir) ) {
		char *aux;
		asprintf(aux, "edg_wll_MaildirInit(): %s", lbm_errdesc);
		glite_jpcl_SetError(ctx, errno, aux);
		free(aux);
		return -1;
	}

	/* TODO: Pack all the files into one tar file */
	file = files[0];

	if ( ctx->jpps )
		asprintf(msg, "jobid\t%s\nfile\t%s\nproxy\t%sjpps\t%s",
				jobid, file, proxy, ctx->jpps);
	else
		asprintf(msg, "jobid\t%s\nfile\t%s\nproxy\t%s",
				jobid, file, proxy);

	if ( edg_wll_MaildirStoreMsg(ctx->lbmd_dir, "localhost", msg) ) {
		char *aux;
		asprintf(aux, "edg_wll_MaildirStoreMsg(): %s", lbm_errdesc);
		glite_jpcl_SetError(ctx, errno, aux);
		free(aux);
		return -1;
	}

	return 0;
}
