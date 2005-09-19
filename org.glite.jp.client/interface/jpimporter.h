#ifndef __GLITE_JPIMPORTER
#define __GLITE_JPIMPORTER

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _glite_jpimp_context_t *glite_jpimp_context_t;

typedef enum _glite_jpimp_ctx_param_t {
	GLITE_JPIMP_PARAM_JPPS,
	GLITE_JPIMP_PARAM_LBMAILDIR
} glite_jpimp_ctx_param_t;

extern int glite_jpimp_InitContext(glite_jpimp_context_t *);
extern void glite_jpimp_FreeContext(glite_jpimp_context_t);

extern int glite_jpimp_SetParam(
		glite_jpimp_context_t ctx,
		int param, ... );

extern int glite_jpimp_Error(
		glite_jpimp_context_t ctx,
		char **errt,
		char **errd);

extern int glite_jpimporter_upload_files(
		glite_jpimp_context_t ctx,
		char *jobid,
		char *files,
		char *user);

#ifdef __cplusplus
}
#endif

#endif
