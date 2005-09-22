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
