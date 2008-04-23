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
