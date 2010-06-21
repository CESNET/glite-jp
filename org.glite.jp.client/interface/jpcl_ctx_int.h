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

#ifndef __GLITE_JPCLIENT_CONTEXT_INT
#define __GLITE_JPCLIENT_CONTEXT_INT

#ifdef __cplusplus
extern "C" {
#endif

struct _glite_jpcl_context_t {
	int		errCode;
	char   *errDesc;

	char   *jpps;
	char   *lbmd_dir;
};

extern int glite_jpcl_SetError(glite_jpcl_context_t, int, const char *);
extern int glite_jpcl_ResetError(glite_jpcl_context_t);

#ifdef __cplusplus
}
#endif

#endif
