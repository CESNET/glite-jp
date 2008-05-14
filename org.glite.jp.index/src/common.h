#ident "$Header$"

#ifndef GLITE_JPIS_COMMON_H
#define GLITE_JPIS_COMMON_H

#include <glite/jp/types.h>
#include <glite/jp/context.h>

void glite_jpis_trim(char *str);

int glite_jpis_stack_error_source(glite_jp_context_t ctx, int code, const char *func, int line, const char *desc, ...);

#define glite_jpis_stack_error(CTX, CODE, DESCFMT...) glite_jpis_stack_error_source((CTX), (CODE), __FUNCTION__, __LINE__, ##DESCFMT);

#endif
