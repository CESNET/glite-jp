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

#ifndef __GLITE_JP_CLIENT__
#define __GLITE_JP_CLIENT__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _glite_jpcl_context_t *glite_jpcl_context_t;

typedef enum _glite_jpcl_ctx_param_t {
	GLITE_JPCL_PARAM_JPPS,
	GLITE_JPCL_PARAM_LBMAILDIR
} glite_jpcl_ctx_param_t;

extern int glite_jpcl_InitContext(glite_jpcl_context_t *);
extern void glite_jpcl_FreeContext(glite_jpcl_context_t);

extern int glite_jpcl_SetParam(
		glite_jpcl_context_t ctx,
		int param, ... );

extern int glite_jpcl_Error(
		glite_jpcl_context_t ctx,
		char **errt,
		char **errd);

#ifdef __cplusplus
}
#endif

#endif
