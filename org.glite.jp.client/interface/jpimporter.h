#ifndef __GLITE_JPIMPORTER__
#define __GLITE_JPIMPORTER__

#ifndef GLITE_REG_IMPORTER_MDIR
#define GLITE_REG_IMPORTER_MDIR		"/tmp/jpreg"
#endif 

#ifndef GLITE_DUMP_IMPORTER_MDIR
#define GLITE_DUMP_IMPORTER_MDIR	"/tmp/jpdump"
#endif 

#ifndef GLITE_SANDBOX_IMPORTER_MDIR
#define GLITE_SANDBOX_IMPORTER_MDIR	"/tmp/jpsandbox"
#endif 

#define PERF_JOBID_START_PREFIX "https://start.megajob/START-"
#define PERF_JOBID_STOP_PREFIX "https://stop.megajob/STOP-"
#define PERF_START_FILE		"/tmp/jp_megajob_start"
#define PERF_STOP_FILE_FORMAT   "/tmp/jp_megajob_%s"

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
