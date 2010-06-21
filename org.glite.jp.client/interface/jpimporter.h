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

#ifndef __GLITE_JPIMPORTER__
#define __GLITE_JPIMPORTER__

#ifndef GLITE_REG_IMPORTER_MDIR
#define GLITE_REG_IMPORTER_MDIR		"/var/glite/jpreg"
#endif 

#ifndef GLITE_DUMP_IMPORTER_MDIR
#define GLITE_DUMP_IMPORTER_MDIR	"/var/glite/jpdump"
#endif 

#ifndef GLITE_SANDBOX_IMPORTER_MDIR
#define GLITE_SANDBOX_IMPORTER_MDIR	"/var/glite/jpsandbox"
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
