#ident "$Header$"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdsoap2.h>

#include <glite/jp/types.h>
#include <glite/jp/context.h>

#include "common.h"

#define WHITE_SPACE_SET "\n\r \t"


void glite_jpis_trim_soap(struct soap *soap, char **soap_str) {
	size_t pos, len;
	char *s;

	if (!*soap_str) return;

	pos = strspn(*soap_str, WHITE_SPACE_SET);
	len = strcspn(*soap_str + pos, WHITE_SPACE_SET);
	s = soap_malloc(soap, len + 1);
	memcpy(s, *soap_str + pos, len);
	s[len] = '\0';

	soap_dealloc(soap, *soap_str);
	*soap_str = s;
}


int glite_jpis_stack_error_source(glite_jp_context_t ctx, int code, const char *func, int line, const char *descfmt, ...) {
	glite_jp_error_t err;
	char *source, *desc;
	va_list ap;
	
	va_start(ap, descfmt);

	asprintf(&source, "%s:%d", func, line);
	if (descfmt) vasprintf(&desc, descfmt, ap);
	else desc = NULL;
	memset(&err, 0, sizeof err);
	err.code = code;
	err.desc = desc;
	err.source = source;
	glite_jp_stack_error(ctx, &err);
	free(source);
	free(desc);

	va_end(ap);
	return code;
}
