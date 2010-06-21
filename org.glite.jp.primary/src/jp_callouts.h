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

#include <gssapi.h>
#include "glite/jp/context.h"

/* must be named this way to provide the name expected by the globus_gsi_authz
 * header and its typedef of globus_gsi_authz_handle_t */
typedef struct globus_i_gsi_authz_handle_s {
   gss_ctx_id_t gss_ctx;
} globus_i_gsi_authz_handle_s;

typedef struct authz_jp_system_state_struct {
   glite_jp_context_t jp_ctx;
} authz_jp_system_state_struct;
