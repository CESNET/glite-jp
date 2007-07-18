#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "jp_client.h"
#include "jpimporter.h" 
#include "glite/lb/lb_maildir.h" 
#include "glite/wmsutils/jobid/cjobid.h" 
#include "glite/wmsutils/jobid/cjobid.h"


#define USER "Job Generator Buddy" 
#define BKSERVER "funny.zcu.cz"
#define BKPORT 9000
#ifndef EDG_DUMP_STORAGE
#define EDG_DUMP_STORAGE	"/tmp/dump"
#endif
#ifndef EDG_PURGE_STORAGE
#define EDG_PURGE_STORAGE	"/tmp/purge"
#endif


char *jpreg_dir;
char *dump_dir;
char *user;
int do_exit = 0;
int perf_regs, perf_dumps;
char perf_ts[100];


static int register_init();
static int register_add(const char *jobid);
static void get_time(char *s, size_t maxs, double *t);
static int dump_init();


static void handler(int sig) {
	do_exit = sig;
	signal(sig, SIG_DFL);
}

int main(int argc, char *argv[]) {
	char start_jobid[256], stop_jobid[256];
	double ts, ts2;
	int ret;

	get_time(perf_ts, sizeof(perf_ts), &ts);
	snprintf(start_jobid, sizeof(start_jobid), PERF_JOBID_START_PREFIX "%s", perf_ts);
	snprintf(stop_jobid, sizeof(stop_jobid), PERF_JOBID_STOP_PREFIX "%s", perf_ts);

	if ((ret = register_init()) != 0) return ret;
	if ((ret = dump_init()) != 0) return ret;
	if ((ret = register_add(start_jobid)) != 0) return ret;
	if (signal(SIGINT, handler) == SIG_ERR) {
		ret = errno;
		fprintf(stderr, "%s: can't set signal handler: %s\n", __FUNCTION__, strerror(errno));
		return ret;
	}
	printf("%s\n", start_jobid);
	printf("start: %lf\n", ts);
	while (!do_exit) {
		if ((ret = register_add(NULL)) != 0) return ret;
		if (argc > 1)
			if ((ret = dump_add(argv[1])) != 0) return ret;
	}
	if ((ret = register_add(stop_jobid)) != 0) return ret;
	get_time(NULL, -1, &ts2);
	printf("stop:  %lf\n", ts2);
	printf("regs:  %d (%lf jobs/day)\n", perf_regs, 86400.0 * perf_regs / (ts2-ts));
	printf("dumps: %d (%lf jobs/day)\n", perf_dumps, 86400.0 * perf_dumps / (ts2-ts));
	printf("%s\n", stop_jobid);

	return 0;
}


static void get_time(char *s, size_t maxs, double *t) {
	struct timeval tv;
	struct tm tm;

	gettimeofday(&tv, NULL);
	if (t) *t = tv.tv_sec + (double)tv.tv_usec / 1000000.0;
	gmtime_r(&tv.tv_sec, &tm);
	if (s && maxs > 0) strftime(s, maxs, "%FT%TZ", &tm);
}


static int register_init() {
        char *env;

        env = getenv("GLITE_LB_EXPORT_JPREG_MAILDIR");
        if (!env) env = GLITE_REG_IMPORTER_MDIR;
        jpreg_dir = strdup(env);

	// TODO: better from certificate        
        env = getenv("GLITE_USER");
        if (!env) env = USER;
        user = strdup(env);

        if (edg_wll_MaildirInit(jpreg_dir) != 0) {
                fprintf(stderr, "maildir init on %s failed\n", jpreg_dir);
                return EIO;
        }

	perf_regs = 0;
        return 0;       
}


static int register_add(const char *jobid) {
        edg_wlc_JobId j;
        char *tmpjobid, *msg;

	if (!jobid) {
		if (edg_wlc_JobIdCreate(BKSERVER, BKPORT, &j) != 0 || (tmpjobid = edg_wlc_JobIdUnparse(j)) == NULL) {
        	        fprintf(stderr, "Can't create jobid\n");
			return EIO;
	        }
        	edg_wlc_JobIdFree(j);
	} else tmpjobid = strdup(jobid);
        asprintf(&msg, "%s\n%s", tmpjobid, user);
	free(tmpjobid);
        if (edg_wll_MaildirStoreMsg(jpreg_dir, BKSERVER, msg) != 0) {
                fprintf(stderr, "Can't store message: %s\n", lbm_errdesc);
                return EIO;
        }
        free(msg);

	perf_regs++;
	return 0;
}


static int dump_init() {
        char *env;

	// FIXME: is it OK? (probably different HEAD and branch)
        env = getenv("GLITE_LB_EXPORT_DUMPDIR");
        if (!env) env = EDG_DUMP_STORAGE;
        dump_dir = strdup(env);
	mkdir(dump_dir, 0755);
	perf_dumps = 0;

        return 0;       
}


static int dump_add(const char *filename) {
	char *fn;
	int ret;

	asprintf(&fn, "%s/mill-test-%s-%06d", dump_dir, perf_ts, perf_dumps);
	if ((ret = link(filename, fn)) != 0) {
                fprintf(stderr, "Can't link file: %s\n", strerror(errno));
	}
	free(fn);

	perf_dumps++;
	return ret;
}
