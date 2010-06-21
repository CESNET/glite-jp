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

#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "jp_client.h"
#include "jpcl_ctx_int.h"
#include "jpimporter.h"


int glite_jpcl_InitContext(glite_jpcl_context_t *ctx)
{
	glite_jpcl_context_t out = (glite_jpcl_context_t) malloc(sizeof(*out));
	if (!out) return ENOMEM;
	memset(out, 0, sizeof(*out));
	assert(out->errDesc == NULL);

	*ctx = out;
	return 0;
}

void glite_jpcl_FreeContext(glite_jpcl_context_t ctx)
{
	free(ctx->jpps);
	free(ctx->lbmd_dir);
}

int glite_jpcl_SetParam(glite_jpcl_context_t ctx, int param, ...)
{
	va_list ap;

	va_start(ap, param);
	switch ( param ) {
	case GLITE_JPCL_PARAM_JPPS:
		if ( ctx->jpps ) free(ctx->jpps);
		ctx->jpps = va_arg(ap, char *);
		ctx->jpps = strdup(ctx->jpps);
		break;
	case GLITE_JPCL_PARAM_LBMAILDIR:
		if ( ctx->lbmd_dir ) free(ctx->lbmd_dir);
		ctx->lbmd_dir = strdup(va_arg(ap, char *));
		break;
	default:
		return glite_jpcl_SetError(ctx, EINVAL, "unknown parameter");
	}

	return 0;
}

int glite_jpcl_Error( glite_jpcl_context_t ctx, char **errt, char **errd)
{
	if ( errt ) *errt = strdup(strerror(ctx->errCode));
	if ( errd ) *errd = (ctx->errDesc)? strdup(ctx->errDesc): NULL;
	return ctx->errCode;
}

int glite_jpcl_SetError(glite_jpcl_context_t ctx, int code, const char *desc)
{
	glite_jpcl_ResetError(ctx);
    if ( code ) {
		ctx->errCode = code;
		if ( desc ) ctx->errDesc = (char *) strdup(desc);
	}

	return ctx->errCode;
}

int glite_jpcl_ResetError(glite_jpcl_context_t ctx)
{
	if ( ctx->errDesc ) free(ctx->errDesc);
	ctx->errDesc = NULL;
	ctx->errCode =  0;

	return ctx->errCode;
}
