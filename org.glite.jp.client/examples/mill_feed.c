#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
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
char *dump; char **dump_index; size_t dump_tokens;


static int register_init();
static int register_add(const char *jobid, char **new_jobid);
static void get_time(char *s, size_t maxs, double *t);
static int dump_init(const char *start_jobid, const char *filenmae);
static int dump_add(const char *filename, const char *jobid);
static void dump_done();


static void handler(int sig) {
	do_exit = sig;
	signal(sig, SIG_DFL);
}


int main(int argc, char *argv[]) {
	char start_jobid[256], stop_jobid[256], *fn;
	double ts, ts2;
	int ret;
	FILE *f;
	char *jobid;

	get_time(perf_ts, sizeof(perf_ts), &ts);
	snprintf(start_jobid, sizeof(start_jobid), PERF_JOBID_START_PREFIX "%s", perf_ts);
	snprintf(stop_jobid, sizeof(stop_jobid), PERF_JOBID_STOP_PREFIX "%s", perf_ts);

	if ((ret = register_init()) != 0) return ret;
	if ((ret = dump_init(start_jobid, argc > 1 ? argv[1] : NULL)) != 0) return ret;
	if ((ret = register_add(start_jobid, NULL)) != 0) return ret;
	if (signal(SIGINT, handler) == SIG_ERR) {
		ret = errno;
		fprintf(stderr, "%s: can't set signal handler: %s\n", __FUNCTION__, strerror(errno));
		return ret;
	}
	printf("%s\n", start_jobid);
	printf("start: %lf\n", ts);
	while (!do_exit) {
		if ((ret = register_add(NULL, &jobid)) != 0) return ret;
		if (argc > 1)
			if ((ret = dump_add(argv[1], jobid)) != 0) return ret;
		free(jobid);
	}
	asprintf(&fn, PERF_STOP_FILE_FORMAT, perf_ts);
	if ((f = fopen(fn, "wt")) == NULL) {
		ret = errno;
		free(fn);
		fprintf(stderr, "Can' create file '%s': %s\n", fn, strerror(errno));
		return ret;
	}
	free(fn);
	fprintf(f, "regs\t%d\n", perf_regs);
	fprintf(f, "dumps\t%d\n", perf_dumps);
	fclose(f);
	if ((ret = register_add(stop_jobid, NULL)) != 0) return ret;
	dump_done();

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


static int register_add(const char *jobid, char **new_jobid) {
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
	if (new_jobid) *new_jobid = tmpjobid;
	else free(tmpjobid);
        if (edg_wll_MaildirStoreMsg(jpreg_dir, BKSERVER, msg) != 0) {
                fprintf(stderr, "Can't store message: %s\n", lbm_errdesc);
                return EIO;
        }
        free(msg);

	perf_regs++;
	return 0;
}


static int dump_init(const char *start_jobid, const char *filename) {
        char *env, *ptr, *delim;
	FILE *f;
	long ssize;
	size_t i, dump_maxtokens, size;
	int ret;

	unlink(PERF_START_FILE);

	dump = NULL;
	dump_index = NULL;
	dump_tokens = 0;
	if (filename) {
		if ((f = fopen(filename, "rt")) == NULL) {
			fprintf(stderr, "Can't open '%s': %s\n", filename, strerror(errno));
			return EIO;
		}
		if (fseek(f, 0, SEEK_END) == -1 || (ssize = ftell(f)) == -1 || fseek(f, 0, SEEK_SET) == -1) {
			fprintf(stderr, "Can't get position in '%s': %s\n", filename, strerror(errno));
			return EIO;
		}
		dump = malloc(size = ssize);
		if (fread(dump, size, 1, f) != 1) {
			ret = errno;
			fprintf(stderr, "Error reading %ld bytes from file: %s\n", ssize, strerror(errno));
			return ret;
		}
		fclose(f);

		dump_maxtokens = 1024;
		dump_index = malloc(sizeof(char *) * dump_maxtokens);
		i = 0;
		ptr = dump;
		do {
			if (dump_tokens >= dump_maxtokens) {
				dump_maxtokens *= 2;
				dump_index = realloc(dump_index, sizeof(char *) * dump_maxtokens);
			}
			delim = strstr(ptr, "DG.JOBID=\"");
			if (delim != ptr) {
				dump_index[dump_tokens++] = ptr;
				if (delim) {
					delim[10] = '\0';
					ptr = delim + 11;
				} else ptr = NULL;
			}
			if (ptr) ptr = strchr(ptr, '\"');
		} while (ptr && ptr[0]);
	}
//for (i = 0; i < dump_tokens; i++) printf("####%s\n", dump_index[i]);

	// FIXME: is it OK? (probably different HEAD and branch)
        env = getenv("GLITE_LB_EXPORT_DUMPDIR");
        if (!env) env = EDG_DUMP_STORAGE;
        dump_dir = strdup(env);
	mkdir(dump_dir, 0755);
	perf_dumps = 0;

	if ((f = fopen(PERF_START_FILE, "wt")) == NULL) {
		fprintf(stderr, "Can't create file '" PERF_START_FILE "': %s\n", strerror(errno));
		return EIO;
	}
	if (start_jobid) fprintf(f, "%s\n", start_jobid);
	fclose(f);

        return 0;
}


static int dump_add(const char *filename, const char *jobid) {
	char *fn;
	int ret;
	size_t i;
	FILE *f;

	ret = 0;
	asprintf(&fn, "%s/mill-test-%s-%06d", dump_dir, perf_ts, perf_dumps);
	if ((f = fopen(fn , "wt")) == NULL) {
		ret = errno;
		fprintf(stderr, "Can't create file '%s': %s\n", fn, strerror(errno));
		goto err;
	}
	for (i = 0; i < dump_tokens; i++) {
		if (fputs(dump_index[i], f) == EOF || (i + 1 < dump_tokens && (fputs(jobid, f) == EOF))) {
			ret = errno;
			fprintf(stderr, "Can't write to '%s': %s\n", fn, strerror(errno));
			goto err_close;
		}
	}

	perf_dumps++;
err_close:
	fclose(f);
err:
	free(fn);
	return ret;
}


static void dump_done() {
	free(dump_index);
	free(dump);
	dump_tokens = 0;
}
