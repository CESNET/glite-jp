#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "jpps_H.h"
#include "jptype_map.h"

int glite_jpps_authz(glite_jp_context_t ctx,int op,const char *job,const char *owner)
{
	glite_jp_error_t	err;
	char	buf[200];
	int	i;

	memset(&err,0,sizeof err);
	glite_jp_clear_error(ctx);
	err.source = __FUNCTION__;
	err.code = EPERM;
	
	switch (op) {
		case SOAP_TYPE___jpsrv__RegisterJob:
		case SOAP_TYPE___jpsrv__StartUpload:
		case SOAP_TYPE___jpsrv__CommitUpload:
			for (i=0; ctx->trusted_peers && ctx->trusted_peers[i]; i++) 
				if (!strcmp(ctx->trusted_peers[i],ctx->peer)) return 0;
			err.desc = "you are not a trusted peer";
			return glite_jp_stack_error(ctx,&err);

		case SOAP_TYPE___jpsrv__GetJobFiles:
		case SOAP_TYPE___jpsrv__GetJobAttributes:
		case SOAP_TYPE___jpsrv__RecordTag:
			assert(owner);
			if (strcmp(owner,ctx->peer)) {
				err.desc = "you are not a job owner";
				glite_jp_stack_error(ctx,&err);
				return 1;
			}
			return 0;
			break;

		default:
			snprintf(buf,sizeof buf,"%d: unknown operation",op);
			err.desc = buf;
			err.code = EINVAL;
			return glite_jp_stack_error(ctx,&err);
	}
}

int glite_jpps_readauth(glite_jp_context_t ctx,const char *file)
{
	FILE	*f = fopen(file,"r");
	glite_jp_error_t	err;
	int	cnt = 0;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;
	if (!f) {
		err.code = errno;
		err.desc = file;
		return glite_jp_stack_error(ctx,&err);
	}

	ctx->trusted_peers = NULL;
	while (!feof(f)) {
		char	buf[BUFSIZ];

		if (fscanf(f,"%[^\n]\n",buf) != 1) {
			err.code = EINVAL;
			err.desc = file;
			fclose(f);
			return glite_jp_stack_error(ctx,&err);
		}

		ctx->trusted_peers = realloc(ctx->trusted_peers, (cnt+2) * sizeof *ctx->trusted_peers);
		ctx->trusted_peers[cnt++] = strdup(buf);
		ctx->trusted_peers[cnt] = NULL;
	}
	fclose(f);
	return 0;
}

