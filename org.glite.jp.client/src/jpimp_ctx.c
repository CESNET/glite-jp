#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "jpimporter.h"
#include "jpimp-ctx-int.h"


int glite_jpimp_InitContext(glite_jpimp_context_t *ctx)
{
	glite_jpimp_context_t out = (glite_jpimp_context_t) malloc(sizeof(*out));
	if (!out) return ENOMEM;
	memset(out, 0, sizeof(*out));
	assert(out->errDesc == NULL);

	*ctx = out;
	return 0;
}

void glite_jpimp_FreeContext(glite_jpimp_context_t ctx)
{
	free(ctx->jpps);
	free(ctx->lbmd_dir);
}

int glite_jpimp_SetParam(glite_jpimp_context_t ctx, int param, ...)
{
	va_list ap;

	va_start(ap, param);
	switch ( param ) {
	case GLITE_JPIMP_PARAM_JPPS:
		if ( ctx->jpps ) free(ctx->jpps);
		ctx->jpps = va_arg(ap, char *);
		ctx->jpps = strdup(ctx->jpps);
		break;
	case GLITE_JPIMP_PARAM_LBMAILDIR:
		if ( ctx->lbmd_dir ) free(ctx->lbmd_dir);
		ctx->lbmd_dir = strdup(va_arg(ap, char *));
		break;
	default:
		return glite_jpimp_SetError(ctx, EINVAL, "unknown parameter");
	}

	return 0;
}

int glite_jpimp_Error(
	glite_jpimp_context_t	ctx,
	char				  **errt,
	char				  **errd)
{
	if ( errt ) *errt = strdup(strerror(ctx->errCode));
	if ( errd ) *errd = (ctx->errDesc)? strdup(ctx->errDesc): NULL;
	return ctx->errCode;
}

int glite_jpimp_SetError(
	glite_jpimp_context_t	ctx,
	int					code,
	const char		   *desc)
{
	glite_jpimp_ResetError(ctx);
    if ( code ) {
		ctx->errCode = code;
		if ( desc ) ctx->errDesc = (char *) strdup(desc);
	}

	return ctx->errCode;
}

int glite_jpimp_ResetError(
	glite_jpimp_context_t ctx)
{
	if ( ctx->errDesc ) free(ctx->errDesc);
	ctx->errDesc = NULL;
	ctx->errCode =  0;

	return ctx->errCode;
}
