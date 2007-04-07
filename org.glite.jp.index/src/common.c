#ident "$Header$"

#include <stdio.h>
#include <string.h>
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


int glite_jpis_stack_error_source(glite_jp_context_t ctx, int code, const char *desc, const char *func, int line) {
	glite_jp_error_t err;
	char *source;
	
	asprintf(&source, "%s:%d", func, line);
	memset(&err, 0, sizeof err);
	err.code = code;
	err.desc = desc;
	err.source = source;
	glite_jp_stack_error(ctx, &err);
	free(source);

	return code;
}
