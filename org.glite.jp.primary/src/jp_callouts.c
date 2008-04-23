#include <stdlib.h>
#include <globus_common.h>
#include <globus_gsi_authz.h>
#include <globus_gsi_authz_callout_error.h>

#include "glite/lbu/trio.h"
#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/db.h"

#include "jp_callouts.h"

#define FTPBE_DEFAULT_DB_CS     "jpps/@localhost:jpps"

/*
   This file provides following authorization callouts used by the globus
   gridftp server. The callouts must be specified in the gsi_auth.confs
    configuration file.
   1. GLOBUS_GSI_AUTHZ_SYSTEM_INIT
     - called upon request arrival, runs under the deamon uid
     - opens connection to the DB
   2. globus_mapping
     - performs mapping to a local account
     - n-to-one mapping implemented, which uses a generic user name
   3. GLOBUS_GSI_AUTHZ_HANDLE_INIT
     - runs under the mapped client uid
     - used to save current GSS context for later use
   4. GLOBUS_GSI_AUTHORIZE_ASYNC
     - called just before the request is served
     - performs actual authZ decision based on current state of the file in
       in the JP DB
   5. GLOBUS_GSI_AUTHZ_HANDLE_DESTROY
     - cleanup of saved data
   (6. GLOBUS_GSI_AUTHZ_SYSTEM_DESTROY)
     - cleanup
     - we should close the connection to the DB, however this callout doesn't
       seem to be called by the ftpd
*/

static globus_result_t
query_db(glite_jp_context_t ctx, glite_lbu_Statement *res,
         const char *format, ...)
{
    char *stmt = NULL;
    int ret;
    glite_lbu_Statement db_res;
    va_list ap;
    globus_result_t result = GLOBUS_FAILURE;

    va_start(ap, format);
    trio_vasprintf(&stmt, format, ap);
    if (stmt == NULL) {
       GLOBUS_GSI_AUTHZ_CALLOUT_ERRNO_ERROR(result, errno);
       return result;
    }
    va_end(ap);

    ret = glite_jp_db_ExecSQL(ctx, stmt, &db_res);
    if (ret <= 0) {
       GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
          result,
          GLOBUS_GSI_AUTHZ_CALLOUT_AUTHZ_DENIED_BY_CALLOUT,
          ((ret == 0) ? "No such file registered" :
                        "Internal error: backend DB access failed"));
       goto end;
    }

    *res = db_res;
    result = GLOBUS_SUCCESS;

end:
    if (stmt)
       free(stmt);

    return result;
}

static globus_result_t
authz_read(authz_jp_system_state_struct *state, char *object, char *client)
{
	globus_result_t result = GLOBUS_FAILURE;
	int db_retn;
	glite_lbu_Statement db_res;
	char *db_row[1] = { NULL };

        result = query_db(state->jp_ctx, &db_res,
                          "select j.owner from jobs j,files f where "
                          "f.ext_url='%|Ss' and j.jobid=f.jobid", object);
        if (result != GLOBUS_SUCCESS) {
            /* XXX clear error stack ?*/
            result = query_db(state->jp_ctx, &db_res,
                               "select j.owner from jobs j,files f where "
                               "f.ext_url='gsi%|Ss' and j.jobid=f.jobid", object);
        }
        if (result != GLOBUS_SUCCESS)
            return result;

	db_retn = glite_jp_db_FetchRow(state->jp_ctx, db_res, 1, NULL, db_row);
	if (db_retn != 1) {
		result = GLOBUS_FAILURE;
		glite_jp_db_FreeStmt(&db_res);
		GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
		   result,
		   GLOBUS_GSI_AUTHZ_CALLOUT_AUTHZ_DENIED_BY_CALLOUT,
		   ("Internal error: backend DB access failed"));
		goto out;
	}
	glite_jp_db_FreeStmt(&db_res);

	if (!strcmp(db_row[0], client)) {
		result = GLOBUS_SUCCESS;
	} else {
		result = GLOBUS_FAILURE;
		GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
		   result,
		   GLOBUS_GSI_AUTHZ_CALLOUT_AUTHZ_DENIED_BY_CALLOUT,
		   ("Permission denied"));
	}

out:
	free(db_row[0]);
	return result;
}

static globus_result_t
authz_write(authz_jp_system_state_struct *state, char *object, char *client)
{
	globus_result_t result = GLOBUS_FAILURE;
	int db_retn;
	glite_lbu_Statement db_res;
	char *db_row[1] = { NULL };

	result = query_db(state->jp_ctx, &db_res,
                           "select state from files where ext_url='%|Ss' and ul_userid='%|Ss'",
                           object, client);
        if (result != GLOBUS_SUCCESS) {
            /* XXX clear error stack ? */
            result = query_db(state->jp_ctx, &db_res,
                              "select state from files where ext_url='gsi%|Ss' and ul_userid='%|Ss'",
                              object, client);
        }
        if (result != GLOBUS_SUCCESS) {
            return result;
        }

	db_retn = glite_jp_db_FetchRow(state->jp_ctx, db_res, 1, NULL, db_row);
	if (db_retn != 1) {
		glite_jp_db_FreeStmt(&db_res);
                result = GLOBUS_FAILURE;
		GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
                  result,
                  GLOBUS_GSI_AUTHZ_CALLOUT_AUTHZ_DENIED_BY_CALLOUT,
		  ("Internal error: backend DB access failed"));
		goto out;
	}
	glite_jp_db_FreeStmt(&db_res);

	if (!strcmp(db_row[0], "uploading")) {
		result = GLOBUS_SUCCESS;
	} else {
                result = GLOBUS_FAILURE;
		GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
		   result,
		   GLOBUS_GSI_AUTHZ_CALLOUT_AUTHZ_DENIED_BY_CALLOUT,
		   ("Upload not in progress"));
	}

out:
	free(db_row[0]);
	return result;
}

static globus_result_t
authz_del(authz_jp_system_state_struct *state, char *object, char *client)
{
    globus_result_t result = GLOBUS_FAILURE;

    GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
	result,
	GLOBUS_GSI_AUTHZ_CALLOUT_AUTHZ_DENIED_BY_CALLOUT,
	("Deleting files not supported"));
    return result;
}

static int
get_client(gss_ctx_id_t ctx, char **name)
{
    gss_buffer_desc token = GSS_C_EMPTY_BUFFER;
    OM_uint32 maj_stat, min_stat;
    gss_name_t client_name = GSS_C_NO_NAME;
    int ret;

    maj_stat = gss_inquire_context(&min_stat, ctx, &client_name, NULL, NULL,
				   NULL, NULL, NULL, NULL);
    if (GSS_ERROR(maj_stat)) {
	ret = -1;
	goto end;
    }

    maj_stat = gss_display_name(&min_stat, client_name, &token, NULL);
    if (GSS_ERROR(maj_stat)) {
	ret = -1;
	goto end;
    }

    *name = strdup(token.value);
    ret = 0;

end:
    if (token.length)
	gss_release_buffer(&min_stat, &token);
    if (client_name != GSS_C_NO_NAME)
	gss_release_name(&min_stat, &client_name);

    return ret;
}

globus_result_t
authz_jp_system_init_callout(va_list ap)
{
    void * authz_system_state;
    authz_jp_system_state_struct *state = NULL;
    char *db_cs = NULL;
    globus_result_t result = GLOBUS_FAILURE;
    glite_jp_context_t jp_ctx;

    authz_system_state = va_arg(ap, void *);

    db_cs = getenv("FTPBE_DB_CS");
    if (!db_cs) db_cs = FTPBE_DEFAULT_DB_CS;

    /* XXX the error messages aren't displayed by ftpd on errors */

    glite_jp_init_context(&jp_ctx);

    if (glite_lbu_InitDBContext(((glite_lbu_DBContext *)&jp_ctx->dbhandle)) != 0) {
       GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
		   result,
		   GLOBUS_GSI_AUTHZ_CALLOUT_SYSTEM_ERROR,
		   ("Internal error: backend DB initialization failed"));
       return GLOBUS_FAILURE;
    }

    if (glite_lbu_DBConnect(jp_ctx->dbhandle, db_cs) != 0) {
       GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
		   result,
		   GLOBUS_GSI_AUTHZ_CALLOUT_SYSTEM_ERROR,
		   ("Internal error: backend DB access failed"));
       return GLOBUS_FAILURE;
    }

    state = globus_libc_calloc(1, sizeof(*state));
    if (state == NULL) {
       GLOBUS_GSI_AUTHZ_CALLOUT_ERRNO_ERROR(result, errno);
       return GLOBUS_FAILURE;
    }

    state->jp_ctx = jp_ctx;

    *(authz_jp_system_state_struct **)authz_system_state = state;
    return GLOBUS_SUCCESS;
}

globus_result_t
authz_jp_system_destroy_callout(va_list ap)
{
    void * authz_system_state;
    globus_result_t                 result = GLOBUS_SUCCESS;

    authz_system_state = va_arg(ap, void *);

    /* XXX close the DB here, however this call seems not be called by gridftpd */
    return result;
}

globus_result_t
authz_jp_handle_init_callout(va_list ap)
{
   globus_gsi_authz_handle_t *handle;
   char * service_name;
   gss_ctx_id_t context;
   globus_gsi_authz_cb_t callback;
   void * callback_arg;
   void * authz_system_state;
   globus_result_t result = GLOBUS_FAILURE;

   handle = va_arg(ap, globus_gsi_authz_handle_t *);
   service_name = va_arg(ap, char *);
   context = va_arg(ap, gss_ctx_id_t);
   callback = va_arg(ap,  globus_gsi_authz_cb_t);
   callback_arg = va_arg(ap, void *);
   authz_system_state = va_arg(ap, void *);

   if (handle == NULL) {
      GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(result,
				     GLOBUS_GSI_AUTHZ_CALLOUT_BAD_ARGUMENT_ERROR,
				     ("null handle"));
      goto end;
   }

   *handle = globus_libc_calloc(1, sizeof(**handle));
   if (*handle == NULL) {
      GLOBUS_GSI_AUTHZ_CALLOUT_ERRNO_ERROR(result, errno);
      goto end;
   }

   (*handle)->gss_ctx = context;
   result = GLOBUS_SUCCESS;

end:
   if (callback)
      callback(callback_arg, callback_arg, result);

   return result;
}

globus_result_t
authz_jp_authorize_async_callout(va_list ap)
{
   globus_gsi_authz_handle_t handle;
   char * action;
   char * object;
   globus_gsi_authz_cb_t callback;
   void * callback_arg;
   void * authz_system_state;
   globus_result_t result = GLOBUS_FAILURE;
   char *client = NULL;

   handle = va_arg(ap, globus_gsi_authz_handle_t);
   action = va_arg(ap, char *);
   object = va_arg(ap, char *);
   callback = va_arg(ap,  globus_gsi_authz_cb_t);
   callback_arg = va_arg(ap, void *);
   authz_system_state = va_arg(ap, void *);

   if (action == NULL) {
      GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
	 result,
 	 GLOBUS_GSI_AUTHZ_CALLOUT_BAD_ARGUMENT_ERROR,
	 ("null action"));
      goto end;
   }

   if (object == NULL) {
      GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
	 result,
	 GLOBUS_GSI_AUTHZ_CALLOUT_BAD_ARGUMENT_ERROR,
	 ("null object"));
      goto end;
   }

   if (handle == NULL) {
      GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
	 result,
	 GLOBUS_GSI_AUTHZ_CALLOUT_BAD_ARGUMENT_ERROR,
	 ("null handle"));
      goto end;
   }

   if (handle->gss_ctx == NULL) {
	GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
	   result,
	   GLOBUS_GSI_AUTHZ_CALLOUT_BAD_ARGUMENT_ERROR,
	   ("bad handle"));
	goto end;
   }

   if (authz_system_state == NULL) {
	GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
	   result,
	   GLOBUS_GSI_AUTHZ_CALLOUT_BAD_ARGUMENT_ERROR,
	   ("system state not initialized, probably the GLOBUS_GSI_AUTHZ_SYSTEM_INIT callout isn't handled"));
	goto end;
   }

   get_client(handle->gss_ctx, &client);
   if (client == NULL) {
	GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
	   result,
	   GLOBUS_GSI_AUTHZ_CALLOUT_BAD_ARGUMENT_ERROR,
	   ("cannot identify client"));
	goto end;
   }

//   fprintf(stdout, "   client \"%s\" asking to \"%s\" on \"%s\"\n", client, action, object);

   if (strcmp(action, "create") == 0) {
      result = authz_write(authz_system_state, object, client);
   } else if (strcmp(action, "write") == 0) {
      result = authz_write(authz_system_state, object, client);
   } else if (strcmp(action, "read") == 0) {
      result = authz_read(authz_system_state, object, client);
   } else if (strcmp(action, "delete") == 0) {
      result = authz_del(authz_system_state, object, client);
   } else {
      result = GLOBUS_FAILURE;
      GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
	result,
	GLOBUS_GSI_AUTHZ_CALLOUT_BAD_ARGUMENT_ERROR,
	("unsupported operation"));
   }

end:
   if (callback && result == GLOBUS_SUCCESS)
      callback(callback_arg, handle, result);

   if (client)
      free(client);

   return result;
}

int
authz_jp_handle_destroy_callout(va_list ap)
{
    globus_gsi_authz_handle_t 		handle;
    void * 				authz_system_state;
    int                             	result = (int) GLOBUS_SUCCESS;
    globus_gsi_authz_cb_t		callback;
    void *				callback_arg;

    handle = va_arg(ap, globus_gsi_authz_handle_t);
    callback = va_arg(ap, globus_gsi_authz_cb_t);
    callback_arg = va_arg(ap, void *);
    authz_system_state = va_arg(ap, void *);
    
    if (handle != NULL) {
	globus_libc_free(handle);
    }

    /* XXX 
    glite_jp_db_close((authz_jp_system_state_struct*)authz_system_state->jp_ctx);
    */
    
#if 0
    if (callback)
	callback(callback_arg, handle, result);
#endif
    return result;
}

globus_result_t
authz_jp_globus_mapping(va_list ap)
{
    gss_ctx_id_t                        context;
    char *                              service;
    char *                              desired_identity;
    char *                              identity_buffer;
    unsigned int                        buffer_length;
    char *logname;
    char *client = NULL;
    int ret;
    globus_result_t result = GLOBUS_FAILURE;

    context = va_arg(ap, gss_ctx_id_t);
    service = va_arg(ap, char *);
    desired_identity = va_arg(ap, char *);
    identity_buffer = va_arg(ap, char *);
    buffer_length = va_arg(ap, unsigned int);

    logname = getenv("GLITE_USER");
    if (logname == NULL) {
       GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
           result,
           GLOBUS_GSI_AUTHZ_CALLOUT_CONFIGURATION_ERROR,
           ("the GLITE_USER variable isn't set, can't map user"));
       return GLOBUS_FAILURE;
    }

    if (desired_identity) {
       result = (strcmp(logname, desired_identity) == 0) ? 
                   GLOBUS_SUCCESS : GLOBUS_FAILURE;
       goto end;
    }

    ret = get_client(context, &client);
    if (ret) {
       GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
            result,
            GLOBUS_GSI_AUTHZ_CALLOUT_SYSTEM_ERROR,
            ("can't get client's name"));
       goto end;
    }

    if (strlen(logname) + 1 > buffer_length) {
       GLOBUS_GSI_AUTHZ_CALLOUT_ERROR(
            result,
            GLOBUS_GSI_AUTHZ_CALLOUT_SYSTEM_ERROR,
            ("Not enough space to store mapped identity"));
       goto end;
    }

    strcpy(identity_buffer, logname);
    result = GLOBUS_SUCCESS;
       
end:
    if (client)
       free(client);

    return result;
}
