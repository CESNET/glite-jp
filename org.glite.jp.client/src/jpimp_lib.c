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

#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>


#define COMPILE_WITH_LIBTAR
#ifdef COMPILE_WITH_LIBTAR
#  include <libtar.h>
#endif

#include "glite/lbu/maildir.h"

#include "jp_client.h"
#include "jpimporter.h"
#include "jpcl_ctx_int.h"

#define TEMP_FILE_PREFIX		"/tmp/jpimporter"

int glite_jpimporter_upload_files(
	glite_jpcl_context_t	ctx,
	const char			   *jobid,
	const char			  **files,
	const char			   *proxy)
{
#ifdef COMPILE_WITH_LIBTAR
	TAR		   *t = NULL;
#endif
	char	   *msg = NULL,
			   *errs = NULL;
	char		archive[PATH_MAX];
	int			fd = -1,
				rv = 0,
				i;


	assert((files != NULL) && (files[0] != NULL));
	assert(jobid != NULL);
	/* TODO: get the user proxy if it is not specified and find its location */
	assert(proxy != NULL);

	if ( glite_lbu_MaildirInit(ctx->lbmd_dir) ) {
		asprintf(&errs, "glite_lbu_MaildirInit(): %s", lbm_errdesc);
		glite_jpcl_SetError(ctx, errno, errs);
		free(errs);
		return -1;
	}

	i = 0;
	do {
		if ( ++i > 10 ) {
			glite_jpcl_SetError(ctx, ECANCELED, "Can't create temporary tar file");
			return -1;
		}
		snprintf(archive, PATH_MAX, "%s_%d_%ld.tar",
				TEMP_FILE_PREFIX, getpid(), time(NULL));
		if ( (fd = open(archive, O_CREAT|O_EXCL|O_WRONLY, 00600)) < 0 ) {
			if ( errno == EEXIST ) { sleep(2); continue; }
			asprintf(&errs, "Can't create tar file %s", archive);
			glite_jpcl_SetError(ctx, ECANCELED, errs);
			free(errs);
			return -1;
		}
	} while ( fd < 0 );

#ifdef COMPILE_WITH_LIBTAR
	if ( tar_fdopen(&t, fd, archive, NULL, O_WRONLY, 0, TAR_GNU) < 0 ) {
		asprintf(&errs, "Can't create tar archive %s", archive);
		glite_jpcl_SetError(ctx, errno, errs);
		rv = -1;
		goto cleanup;
	}

	for ( i = 0; files[i]; i++ ) {
		char *s = (char *)files[i];
		if ( tar_append_file(t, s, (s[0]=='/')? s+1: s) < 0 ) {
			asprintf(&errs, "Can't append file into tar archive %s", archive);
			glite_jpcl_SetError(ctx, errno, errs);
			rv = -1;
			goto cleanup;
		}   
	}
#endif

	if ( ctx->jpps )
		asprintf(&msg, "jobid\t%s\nfile\t%s\nproxy\t%s\njpps\t%s\n",
				jobid, archive, proxy, ctx->jpps);
	else
		asprintf(&msg, "jobid\t%s\nfile\t%s\nproxy\t%s\n",
				jobid, archive, proxy);

	if ( glite_lbu_MaildirStoreMsg(ctx->lbmd_dir, "localhost", msg) ) {
		asprintf(&errs, "glite_lbu_MaildirStoreMsg(): %s", lbm_errdesc);
		glite_jpcl_SetError(ctx, errno, errs);
		rv = -1;
		goto cleanup;
	}


cleanup:
#ifdef COMPILE_WITH_LIBTAR
	if ( t ) tar_close(t);
	else close(fd);
#else
	close(fd);
#endif
	if ( rv ) unlink(archive);
	free(errs);
	free(msg);

	return rv;
}
