#ifndef __GLITE_JPIMPORTER_CONTEXT_INT
#define __GLITE_JPIMPORTER_CONTEXT_INT

#ifdef __cplusplus
extern "C" {
#endif

struct _glite_jpimp_context_t {
	int		errCode;
	char   *errDesc;

	char   *jpps;
	char   *lbmd_dir;
};

extern int glite_jpimp_SetError(glite_jpimp_context_t, int, const char *);
extern int glite_jpimp_ResetError(glite_jpimp_context_t);

#ifdef __cplusplus
}
#endif

#endif
