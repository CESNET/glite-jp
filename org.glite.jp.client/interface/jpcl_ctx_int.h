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
