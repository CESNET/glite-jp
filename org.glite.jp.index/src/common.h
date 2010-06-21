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


#ifndef GLITE_JPIS_COMMON_H
#define GLITE_JPIS_COMMON_H

#include <glite/jp/types.h>
#include <glite/jp/context.h>

void glite_jpis_trim(char *str);

int glite_jpis_stack_error_source(glite_jp_context_t ctx, int code, const char *func, int line, const char *desc, ...);

#define glite_jpis_stack_error(CTX, CODE, DESCFMT...) glite_jpis_stack_error_source((CTX), (CODE), __FUNCTION__, __LINE__, ##DESCFMT);

int glite_jp_typeplugin_load(glite_jp_context_t ctx,const char *so);

int glite_jpis_find_attr(char **attrs, const char *attr);

#endif
