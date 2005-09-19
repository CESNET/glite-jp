#include "lb_maildir"
#include "jpimporter.h"
#include "jpimp-ctx-int.h"

int glite_jpimporter_upload_files(
	glite_jpimp_context_t	ctx,
	char				   *jobid,
	char				   *files,
	char				   *userdn)
{
	if ( edg_wll_MaildirInit(ctx->lbmd_dir) ) {
		char *aux;
		asprintf(aux, "Can't initialize maildir structure - %s", lbm_errdesc);
		glite_jpimp_SetError(ctx, errno, aux);
		free(aux);
		return -1;
	}

	edg_wll_MaildirStoreMsg(const char *, const char *, const char *);

	return 0;
}
