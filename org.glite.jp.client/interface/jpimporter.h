#ifndef __GLITE_JPIMPORTER__
#define __GLITE_JPIMPORTER__

#ifdef __cplusplus
extern "C" {
#endif

extern int glite_jpimporter_upload_files(
		glite_jpcl_context_t ctx,
		const char *jobid,
		const char **files,
		const char *proxy);

#ifdef __cplusplus
}
#endif

#endif
