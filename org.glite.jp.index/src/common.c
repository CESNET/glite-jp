#ident "$Header$"
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


#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdsoap2.h>

#include <glite/jp/types.h>
#include <glite/jp/context.h>

#include "common.h"

#define WHITE_SPACE_SET "\n\r \t"


void glite_jpis_trim(char *str) {
	size_t pos, len;

	if (!str) return;

	pos = strspn(str, WHITE_SPACE_SET);
	len = strcspn(str + pos, WHITE_SPACE_SET);
	if (pos) memmove(str, str + pos, len);
	str[len] = '\0';
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


int glite_jpis_find_attr(char **attrs, const char *attr){
	size_t i;

        i = 0;
	while (attrs[i]) {
                if (strcasecmp(attr, attrs[i]) == 0) return 1;
                i++;
        }
        return 0;
}
