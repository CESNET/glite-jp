#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <linux/limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>
#include <fcntl.h>
#include <libgen.h>
#include <ctype.h>

#include "glite/lbu/maildir.h"

#include "jpps_H.h"
#include "jpps_.nsmap"

#include "jptype_map.h"
#include "glite/security/glite_gsplugin.h"
#include "glite/security/glite_gscompat.h"
#include "glite/jp/known_attr.h"

#include "globus_ftp_client.h"
#include "jp_client.h"
#include "jpimporter.h"

#if GSOAP_VERSION <= 20602
#define soap_call___jpsrv__RegisterJob soap_call___ns1__RegisterJob
#endif


typedef struct {
	char	   *key;
	char	   *val;
} msg_pattern_t;

#ifndef dprintf
#define dprintf(FMT, ARGS...)		{ if (debug) printf(FMT, ##ARGS); }
#endif

#define check_soap_fault(SOAP, ERR) glite_jp_clientCheckFault((SOAP), (ERR), name, 1)

#ifndef GLITE_JPIMPORTER_PIDFILE
#define GLITE_JPIMPORTER_PIDFILE	"/var/run/glite-jpimporter.pid"
#endif 

#ifndef GLITE_JPPS
#define GLITE_JPPS					"http://localhost:8901"
#endif 

#define	MAX_REG_CONNS				500
#define JPPS_NO_RESPONSE_TIMEOUT		120
#define JPREG_REPEAT_TIMEOUT			300
#define JPREG_GIVUP_TIMEOUT			3000
#define JP_REPEAT_TIMEOUT			360
#define JP_GIVUP_TIMEOUT			3600
#define PID_POOL_SIZE				20
#define DEFAULT_DUMP_SLAVES_NUMBER		1


static int		debug = 0;
static int		die = 0;
static int		child_died = 0;
static int		poll = 2;
static char		*name;
static char		*jpps = GLITE_JPPS;
static char		reg_mdir[PATH_MAX] = GLITE_REG_IMPORTER_MDIR;
static char		dump_mdir[PATH_MAX] = GLITE_DUMP_IMPORTER_MDIR;
static char		sandbox_mdir[PATH_MAX] = GLITE_SANDBOX_IMPORTER_MDIR;
static char		*store = NULL;
static struct soap	*soap;

static time_t		cert_mtime = 0;
static char	   	*server_cert = NULL,
			*server_key = NULL,
			*cadir = NULL;
static edg_wll_GssCred	mycred = NULL;
#ifdef JP_PERF
typedef struct {
	char *id, *name;
	long int count, limit;
	double start, end;
} perf_t;

int			sink = 0;
perf_t			perf = {name:NULL,};
#endif
static int gftp_initialized = 0;
static globus_ftp_client_handle_t			hnd;


static struct option opts[] = {
	{ "help",        0, NULL,    'h'},
	{ "cert",        1, NULL,    'c'},
	{ "key",         1, NULL,    'k'},
	{ "CAdir",       1, NULL,    'C'},
	{ "debug",       0, NULL,    'g'},
	{ "jpps",        1, NULL,    'p'},
	{ "reg-mdir",    1, NULL,    'r'},
	{ "dump-mdir",   1, NULL,    'd'},
	{ "dump-slaves", 1, NULL,    'D'},
	{ "sandbox-mdir",1, NULL,    's'},
	{ "pidfile",     1, NULL,    'i'},
	{ "poll",        1, NULL,    't'},
	{ "store",       1, NULL,    'S'},
	{ "store",       1, NULL,    'S'},
#ifdef JP_PERF
	{ "perf-sink",   1, NULL,    'K'},
#endif
	{ NULL,          0, NULL,     0}
};

static const char *get_opt_string = "hgp:r:d:D:s:i:t:c:k:C:"
#ifdef JP_PERF
	"K:"
#endif
;

#include "glite/jp/ws_fault.c"

#ifdef JP_PERF
static void stats_init(perf_t *perf, const char *name);
static void stats_set_jobid(perf_t *perf, const char *jobid);
static void stats_get_limit(perf_t *perf, const char *name);
static void stats_done(perf_t *perf);
#endif

static void usage(char *me)
{
	fprintf(stderr,"usage: %s [option]\n"
		"\t-h, --help         displays this screen\n"
		"\t-k, --key          private key file\n"
		"\t-c, --cert         certificate file\n"
		"\t-C, --CAdir        trusted certificates directory\n"
		"\t-g, --debug        don't run as daemon, additional diagnostics\n"
		"\t-p, --jpps         JP primary service server\n"
		"\t-r, --reg-mdir     path to the 'LB maildir' subtree for registrations\n"
		"\t-d, --dump-mdir    path to the 'LB maildir' subtree for LB dumps\n"
		"\t-D, --dump-slaves  number of slaves processing LB dumps\n"
		"\t-s, --sandbox-mdir path to the 'LB maildir' subtree for input/output sandboxes\n"
		"\t-i, --pidfile      file to store master pid\n"
		"\t-t, --poll         maildir polling interval (in seconds)\n"
		"\t-S, --store        keep uploaded jobs in this directory\n"
#ifdef JP_PERF
		"\t-K, --perf-sink    1=stats, 2=without WS calls, 3=stats+without WS\n"
#endif
		, me);
}

static void catchsig(int sig)
{
	die = sig;
}

static void catch_chld(int sig __attribute__((unused)))
{
	child_died = 1;
}


static int slave(int (*)(void), const char *);
static int reg_importer(void);
static int dump_importer(void);
static int sandbox_importer(void);
static int parse_msg(char *, msg_pattern_t []);
static int gftp_put_file(const char *, int);
static int refresh_connection(struct soap *soap);


int main(int argc, char *argv[])
{
	edg_wll_GssStatus	gss_code;
	struct sigaction	sa;
	sigset_t		sset;
	FILE			*fpid;
	pid_t			reg_pid, sandbox_pid;
	pid_t			dump_pids[PID_POOL_SIZE];
	int			dump_slaves = DEFAULT_DUMP_SLAVES_NUMBER, i;
	int			opt;
	char			*name,
				pidfile[PATH_MAX] = GLITE_JPIMPORTER_PIDFILE;
	glite_gsplugin_Context	plugin_ctx;

	name = strrchr(argv[0],'/');
	if (name) name++; else name = argv[0];

	if ( geteuid() )
		snprintf(pidfile, sizeof pidfile, "%s/glite_jpimporter.pid", getenv("HOME"));

	while ( (opt = getopt_long(argc, argv, get_opt_string, opts, NULL)) != EOF )
	switch ( opt ) {
		case 'g': debug = 1; break;
		case 'h': usage(name); return 0;
		case 'c': server_cert = optarg; break;
		case 'k': server_key = optarg; break;
		case 'C': cadir = optarg; break;
		case 'p': jpps = optarg; break;
		case 't': poll = atoi(optarg); break;
		case 'S': store = optarg; break;
		case 'r': strcpy(reg_mdir, optarg); break;
		case 'd': strcpy(dump_mdir, optarg); break;
		case 'D': dump_slaves = atoi(optarg); break;
		case 's': strcpy(sandbox_mdir, optarg); break;
		case 'i': strcpy(pidfile, optarg); break;
#ifdef JP_PERF
		case 'K': sink = atoi(optarg); break;
#endif
		case '?': usage(name); return 1;
	}
	if ( optind < argc ) { usage(name); return 1; }

	if (dump_slaves > PID_POOL_SIZE) {
		fprintf(stderr,"Maximum number of dump slaves is %d\n", PID_POOL_SIZE);
		return(1);
	}

	memset(&dump_pids,0,sizeof(dump_pids));	

	setlinebuf(stdout);
	setlinebuf(stderr);

	fpid = fopen(pidfile,"r");
	if ( fpid ) {
		int opid = -1;

		if ( fscanf(fpid,"%d",&opid) == 1 ) {
			if ( !kill(opid,0) ) {
				fprintf(stderr,"%s: another instance running, pid = %d\n",argv[0],opid);
				return 1;
			}
			else if (errno != ESRCH) { perror("kill()"); return 1; }
		}
		fclose(fpid);
	} else if (errno != ENOENT) { perror(pidfile); return 1; }
	fpid = fopen(pidfile, "w");
	if ( !fpid ) { perror(pidfile); return 1; }
	fprintf(fpid, "%d", getpid());
	fclose(fpid);
		
	glite_lbu_MaildirInit(reg_mdir);
	glite_lbu_MaildirInit(dump_mdir);
	glite_lbu_MaildirInit(sandbox_mdir);
	if (store && *store) {
		if (mkdir(store, 0750) != 0 && errno != EEXIST) {
			fprintf(stderr, "Can't create directory %s: %s\n", store, strerror(errno));
			store = NULL;
		}
	}

	if ( !debug ) {
		if ( daemon(1,0) == -1 ) { perror("deamon()"); exit(1); }

		fpid = fopen(pidfile,"w");
		if ( !fpid ) { perror(pidfile); return 1; }
		fprintf(fpid, "%d", getpid());
		fclose(fpid);
		openlog(name, LOG_PID, LOG_DAEMON);
	} else { setpgid(0, getpid()); }

	dprintf("Master pid %d\n", getpid());

	if ( globus_module_activate(GLOBUS_FTP_CLIENT_MODULE) != GLOBUS_SUCCESS ) {
		dprintf("[master] Could not activate ftp client module\n");
		if (!debug) syslog(LOG_INFO, "Could not activate ftp client module\n");
		exit(1);
	} else dprintf("[master] Ftp client module activated\n");
	
	if ( !server_cert || !server_key )
		fprintf(stderr, "%s: key or certificate file not specified"
						" - unable to watch them for changes!\n", argv[0]);
	if ( cadir ) setenv("X509_CERT_DIR", cadir, 1);
	edg_wll_gss_watch_creds(server_cert, &cert_mtime);
	if ( !edg_wll_gss_acquire_cred_gsi(server_cert, server_key, &mycred, &gss_code) ) {
		dprintf("[master] Server identity: %s\n", mycred->name);
	} else {
		char *errmsg;
		edg_wll_gss_get_error(&gss_code, "edg_wll_gss_acquire_cred_gsi()", &errmsg);
		dprintf("[master] %s\n", errmsg);
		free(errmsg);
		dprintf("[master] Running unauthenticated\n");
	}

	memset(&sa, 0, sizeof(sa)); assert(sa.sa_handler == NULL);
	sa.sa_handler = catchsig;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	sa.sa_handler = catch_chld;
	sigaction(SIGCHLD, &sa, NULL);

	sa.sa_handler = SIG_IGN;
	sigaction(SIGUSR1, &sa, NULL);

	sigemptyset(&sset);
	sigaddset(&sset, SIGCHLD);
	sigaddset(&sset, SIGTERM);
	sigaddset(&sset, SIGINT);
	sigprocmask(SIG_BLOCK, &sset, NULL);

	soap = calloc(1, sizeof *soap);
	soap_init2(soap, SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE);
	soap_set_omode(soap, SOAP_IO_BUFFER);
	soap_set_namespaces(soap, jpps__namespaces);

	glite_gsplugin_init_context(&plugin_ctx);
	glite_gsplugin_set_credential(plugin_ctx, mycred);
	soap_register_plugin_arg(soap, glite_gsplugin,plugin_ctx);

	if ( (reg_pid = slave(reg_importer, "reg-imp")) < 0 ) {
		perror("starting reg importer slave");
		exit(1);
	}
	for (i=0; i < dump_slaves; i++) {
		if ( (dump_pids[i] = slave(dump_importer, "dump-imp")) < 0 ) {
			perror("starting dump importer slave");
			exit(1);
		}
	}
	if ( (sandbox_pid = slave(sandbox_importer, "sandbox-imp")) < 0 ) {
		perror("starting sandbox importer slave");
		exit(1);
	}

	while ( !die ) {

		sigprocmask(SIG_UNBLOCK, &sset, NULL);
		sleep(5);
		sigprocmask(SIG_BLOCK, &sset, NULL);

		if ( child_died ) {
			int     pid;

			while ( (pid = waitpid(-1, NULL, WNOHANG)) > 0 ) {
				if ( !die ) {
					if ( pid == reg_pid ) {
						dprintf("[master] reg importer slave died [%d]\n", pid);
						if (!debug) syslog(LOG_INFO, "reg importer slave died [%d]\n", die);
						if ( (reg_pid = slave(reg_importer, "reg-imp")) < 0 ) {
							perror("starting reg importer slave");
							kill(0, SIGINT);
							exit(1);
						}
						dprintf("[master] reg importer slave restarted [%d]\n", reg_pid);
					} else if ( pid == sandbox_pid ) {
						dprintf("[master] sandbox importer slave died [%d]\n", pid);
						if (!debug) syslog(LOG_INFO, "sandbox importer slave died [%d]\n", die);
						if ( (sandbox_pid = slave(sandbox_importer, "sandbox-imp")) < 0 ) {
							perror("starting sandbox importer slave");
							kill(0, SIGINT);
							exit(1);
						}
						dprintf("[master] sandbox importer slave restarted [%d]\n", sandbox_pid);
					} else /* must be in dump_pids */ {
						dprintf("[master] dump importer slave died [%d]\n", pid);
						if (!debug) syslog(LOG_INFO, "dump importer slave died [%d]\n", die);
						for (i=0; (i < dump_slaves) && (pid != dump_pids[i]); i++);
						assert(i < dump_slaves); // pid should be in pool

						if ( (dump_pids[i] = slave(dump_importer, "dump-imp")) < 0 ) {
							perror("starting dump importer slave");
							kill(0, SIGINT);
							exit(1);
						}
						dprintf("[master] dump importer slave restarted [%d]\n", dump_pids[i]);
				
					}

				}
			}
			child_died = 0;
			continue;
		}
	}

	dprintf("[master] Terminating on signal %d\n", die);
	if (!debug) syslog(LOG_INFO, "Terminating on signal %d\n", die);
	kill(0, die);

    globus_module_deactivate_all();
	unlink(pidfile);

	return 0;
}

static int slave(int (*fn)(void), const char *nm)
{
	struct sigaction	sa;
	sigset_t		sset;
	int			pid,
				conn_cnt = 0;


	if ( (pid = fork()) ) return pid;

	asprintf(&name,"%s %d",nm,getpid());
	memset(&sa, 0, sizeof(sa)); assert(sa.sa_handler == NULL);
	sa.sa_handler = catchsig;
	sigaction(SIGUSR1, &sa, NULL);

	sigemptyset(&sset);
	sigaddset(&sset, SIGTERM);
	sigaddset(&sset, SIGINT);
	sigaddset(&sset, SIGUSR1);
	sigprocmask(SIG_BLOCK, &sset, NULL);

	dprintf("[%s] slave started - pid [%d]\n", name, getpid());

#ifdef JP_PERF
	while ( !die && (conn_cnt < MAX_REG_CONNS || (sink & 1)) ) {
#else
	while ( !die && (conn_cnt < MAX_REG_CONNS) ) {
#endif
		int ret = fn();

		if ( ret > 0 ) conn_cnt++;
		else if ( ret < 0 ) exit(1);
		else if ( ret == 0 ) {
			sigprocmask(SIG_UNBLOCK, &sset, NULL);
			sleep(poll);
			sigprocmask(SIG_BLOCK, &sset, NULL);
		} 
	}

	if ( die ) {
		dprintf("[%s] %d: Terminating on signal %d\n", name, getpid(), die);
		if ( !debug ) syslog(LOG_INFO, "Terminating on signal %d", die);
	}
    dprintf("[%s] Terminating after %d connections\n", name, conn_cnt);
    if ( !debug ) syslog(LOG_INFO, "Terminating after %d connections", conn_cnt);

    if (gftp_initialized--)
	globus_ftp_client_handle_destroy(&hnd);

	exit(0);
}


static int reg_importer(void)
{
	struct _jpelem__RegisterJob			in;
	struct _jpelem__RegisterJobResponse	empty;
	int			ret;
	static int		readnew = 1;
	char	   *msg = NULL,
			   *fname = NULL,
			   *aux;

	if ( readnew ) ret = glite_lbu_MaildirTransStart(reg_mdir, &msg, &fname);
	else ret = glite_lbu_MaildirRetryTransStart(reg_mdir, (time_t)JPREG_REPEAT_TIMEOUT, (time_t)JPREG_GIVUP_TIMEOUT, &msg, &fname);
	if ( !ret ) { 
		readnew = !readnew;
		if ( readnew ) ret = glite_lbu_MaildirTransStart(reg_mdir, &msg, &fname);
		else ret = glite_lbu_MaildirRetryTransStart(reg_mdir, (time_t)JPREG_REPEAT_TIMEOUT, (time_t)JPREG_GIVUP_TIMEOUT, &msg, &fname);
		if ( !ret ) {
			readnew = !readnew;
			return 0;
		}
	}

	if ( ret < 0 ) {
		dprintf("[%s] glite_lbu_MaildirTransStart: %s (%s)\n", name, strerror(errno), lbm_errdesc);
		if ( !debug ) syslog(LOG_ERR, "glite_lbu_MaildirTransStart: %s (%s)", strerror(errno), lbm_errdesc);
		return -1;
	} else if ( ret > 0 ) {
		dprintf("[%s] JP registration request received\n", name);
		if ( !debug ) syslog(LOG_INFO, "JP registration request received\n");

		ret = 0;
		if ( !(aux = strchr(msg, '\n')) ) {
			dprintf("[%s] Wrong format of message!\n", name);
			if ( !debug ) syslog(LOG_ERR, "Wrong format of message\n");
			ret = 0;
		} else do {
			*aux++ = '\0';
			in.job = msg;
			in.owner = aux;
			dprintf("[%s] Registering '%s'\n", name, msg);
			if ( !debug ) syslog(LOG_INFO, "Registering '%s'\n", msg);
#ifdef JP_PERF
			if ((sink & 1)) {
				if (strncasecmp(msg, PERF_JOBID_START_PREFIX, sizeof(PERF_JOBID_START_PREFIX) - 1) == 0) {
					stats_init(&perf, name);
					stats_set_jobid(&perf, msg);
				}
				if (perf.name && !perf.limit) stats_get_limit(&perf, name);
			}
			if (!(sink & 2)) {
#endif
			refresh_connection(soap);
			ret = soap_call___jpsrv__RegisterJob(soap, jpps, "", &in, &empty);
			if ( (ret = check_soap_fault(soap, ret)) ) break;
#ifdef JP_PERF
			} else ret = 0;
			if (perf.name && ret == 0) {
				perf.count++;
				if (perf.limit) {
					dprintf("[%s statistics] done %ld/%ld\n", name, perf.count, perf.limit);
					if (perf.count >= perf.limit) stats_done(&perf);
				} else
					dprintf("[%s statistics] done %ld/no limit\n", name, perf.count);
			}
#endif
		} while (0);
		glite_lbu_MaildirTransEnd(reg_mdir, fname, ret? LBMD_TRANS_FAILED_RETRY: LBMD_TRANS_OK);
		free(fname);
		free(msg);
		return 1;
	}

	return 0;
}

static int dump_importer(void)
{
	struct _jpelem__StartUpload				su_in;
	struct _jpelem__StartUploadResponse		su_out;
	struct _jpelem__CommitUpload			cu_in;
	struct _jpelem__CommitUploadResponse	empty;
	struct _jpelem__RegisterJob			rj_in;
	struct _jpelem__RegisterJobResponse		rj_empty;
	struct _jpelem__GetJobAttributes		gja_in;
	struct _jpelem__GetJobAttributesResponse	gja_out;
	static int		readnew = 1;
	char		   *msg = NULL,
				   *fname = NULL,
				   *bname;
	char                        fspec[PATH_MAX];
	int				ret, retry_upload, jperrno;
	int				fhnd;
	msg_pattern_t	tab[] = {
						{"jobid", NULL},
						{"file", NULL},
						{"jpps", NULL},
						{"proxy", NULL},
						{NULL, NULL}};
#define				_job   0
#define				_file  1
#define				_jpps  2
#define				_proxy 3


	if ( readnew ) ret = glite_lbu_MaildirTransStart(dump_mdir, &msg, &fname);
	else ret = glite_lbu_MaildirRetryTransStart(dump_mdir, (time_t)JP_REPEAT_TIMEOUT, (time_t)JP_GIVUP_TIMEOUT, &msg, &fname);
	if ( !ret ) { 
		readnew = !readnew;
		if ( readnew ) ret = glite_lbu_MaildirTransStart(dump_mdir, &msg, &fname);
		else ret = glite_lbu_MaildirRetryTransStart(dump_mdir, (time_t)JP_REPEAT_TIMEOUT, (time_t)JP_GIVUP_TIMEOUT, &msg, &fname);
		if ( !ret ) {
			readnew = !readnew;
			return 0;
		}
	}

	if ( ret < 0 ) {
		dprintf("[%s] glite_lbu_MaildirTransStart: %s (%s)\n", name, strerror(errno), lbm_errdesc);
		if ( !debug ) syslog(LOG_ERR, "glite_lbu_MaildirTransStart: %s (%s)", strerror(errno), lbm_errdesc);
		return -1;
	}

	dprintf("[%s] dump JP import request received\n", name);
	if ( !debug ) syslog(LOG_INFO, "dump JP import request received");

	soap_begin(soap);

	ret = 0;
	if ( parse_msg(msg, tab) < 0 ) {
		dprintf("[%s] Wrong format of message!\n", name);
		if ( !debug ) syslog(LOG_ERR, "Wrong format of message");
		ret = 0;
	} else do {
		su_in.job = tab[_job].val;
		su_in.class_ = "urn:org.glite.jp.primary:lb";
		su_in.name = NULL;
		su_in.commitBefore = 1000 + time(NULL);
		su_in.contentType = "text/lb";
#ifdef JP_PERF
		if ((sink & 1)) {
		/* statistics started by file, ended by count limit (from the appropriate result fikle) */
			FILE *f;
			char item[200];

			/* starter */
			if (!perf.name) {
				f = fopen(PERF_START_FILE, "rt");
				if (f) {
					stats_init(&perf, name);
					fscanf(f, "%s", item);
					fclose(f);
					unlink(PERF_START_FILE);
					stats_set_jobid(&perf, item);
				} else
					dprintf("[%s statistics]: not started/too much dumps: %s\n", name, strerror(errno));
			}
			if (perf.name && !perf.limit) stats_get_limit(&perf, name);
		}
		if (!(sink & 2)) {
#endif
		retry_upload = 2;
		do {
			dprintf("[%s] Importing LB dump file '%s'\n", name, tab[_file].val);
			if ( !debug ) syslog(LOG_INFO, "Importing LB dump file '%s'\n", msg);
			refresh_connection(soap);
			ret = soap_call___jpsrv__StartUpload(soap, tab[_jpps].val?:jpps, "", &su_in, &su_out);
			if ( (ret = check_soap_fault(soap, ret)) ) {
			/* unsuccessful dump, register job */
				refresh_connection(soap);
				/* check job existence */
				memset(&gja_in, 0, sizeof gja_in);
				memset(&gja_out, 0, sizeof gja_out);
				gja_in.jobid = su_in.job;
				gja_in.attributes = soap_malloc(soap, sizeof(char *));
				gja_in.__sizeattributes = 1;
				gja_in.attributes[0] = GLITE_JP_ATTR_REGTIME;
				ret = soap_call___jpsrv__GetJobAttributes(soap, jpps, "", &gja_in, &gja_out);
				jperrno = glite_jp_clientGetErrno(soap, ret);
				/* no error ==> some application fault from JP */
				if (jperrno == 0) {
					dprintf("[%s] Dump failed when job %s exists\n", name, su_in.job);
					ret = -1;
					break;
				}
				/* other then "job not found" error ==> other problem, don't register */
				if (jperrno != ENOENT && jperrno != -2) {
					ret = check_soap_fault(soap, ret);
					break;
				}
				/* "job not found" error ==> register job */
				refresh_connection(soap);
				rj_in.job = su_in.job;
				rj_in.owner = mycred->name;
				dprintf("[%s] Failsafe registration\n", name);
				dprintf("[%s] \tjobid: %s\n[%s] \towner: %s\n", name, rj_in.job, name, rj_in.owner);
				if ( !debug ) syslog(LOG_INFO, "Failsafe registration '%s'\n",rj_in.job);
				ret = soap_call___jpsrv__RegisterJob(soap, tab[_jpps].val?:jpps, "", &rj_in, &rj_empty);
				if ( (ret = check_soap_fault(soap, ret)) ) break;
				retry_upload--;
				ret = 1;
			}
		} while (ret != 0 && retry_upload > 0);
		if (ret) break;
		dprintf("[%s] Destination: %s\n\tCommit before: %s\n", name, su_out.destination, ctime(&su_out.commitBefore));
		if (su_out.destination == NULL) {
			dprintf("[%s] StartUpload returned NULL destination\n", name);
			if ( !debug ) syslog(LOG_ERR, "StartUpload returned NULL destination");
			ret = 1;
			break;
		}

		if ( (fhnd = open(tab[_file].val, O_RDONLY)) < 0 ) {
			dprintf("[%s] Can't open dump file: %s\n", name, tab[_file].val);
			if ( !debug ) syslog(LOG_ERR, "Can't open dump file: %s", tab[_file].val);
			ret = 1;
			break;
		}
		if ( (ret = gftp_put_file(su_out.destination, fhnd)) ) break;
		close(fhnd);
		dprintf("[%s] File sent, commiting the upload\n", name);
		cu_in.destination = su_out.destination;
		refresh_connection(soap);
		ret = soap_call___jpsrv__CommitUpload(soap, tab[_jpps].val?:jpps, "", &cu_in, &empty);
		if ( (ret = check_soap_fault(soap, ret)) ) break;
		dprintf("[%s] Dump upload succesfull\n", name);
#ifdef JP_PERF
		} else ret = 0;
		if (perf.name && ret == 0) {
			perf.count++;
			if (perf.limit) {
				dprintf("[%s statistics] done %ld/%ld\n", name, perf.count, perf.limit);
				if (perf.count >= perf.limit) stats_done(&perf);
			} else
				dprintf("[%s statistics] done %ld/no limit\n", name, perf.count);
		}
#endif
		if (store && *store) {
			bname = strdup(tab[_file].val);
			snprintf(fspec, sizeof fspec, "%s/%s", store, basename(bname));
			free(bname);
			if (rename(tab[_file].val, fspec) != 0) 
				fprintf(stderr, "moving %s to %s failed: %s\n", tab[_file].val, fspec, strerror(errno));
			else
				dprintf("[%s] moving %s to %s OK\n", name, tab[_file].val, fspec);
		} else {
			if (unlink(tab[_file].val) != 0)
				fprintf(stderr, "removing %s failed: %s\n", tab[_file].val, strerror(errno));
			else
				dprintf("[%s] %s removed\n", name, tab[_file].val);
		}
	} while (0);
	soap_end(soap);

	glite_lbu_MaildirTransEnd(dump_mdir, fname, ret? LBMD_TRANS_FAILED_RETRY: LBMD_TRANS_OK);
	free(fname);
	free(msg);

	return 1;
}


static int sandbox_importer(void)
{
	struct _jpelem__StartUpload		su_in;
	struct _jpelem__StartUploadResponse	su_out;
	struct _jpelem__CommitUpload		cu_in;
	struct _jpelem__CommitUploadResponse	empty;
	static int	readnew = 1;
	char		*msg = NULL,
			*fname = NULL;
	int		ret;
	int		fhnd;
	msg_pattern_t	tab[] = {
				{"jobid", NULL},
				{"file", NULL},
				{"jpps", NULL},
				{"proxy", NULL},
				{NULL, NULL}};

#define			_job   0
#define			_file  1
#define			_jpps  2
#define			_proxy 3


	if ( readnew ) ret = glite_lbu_MaildirTransStart(sandbox_mdir, &msg, &fname);
	else ret = glite_lbu_MaildirRetryTransStart(sandbox_mdir, (time_t)JP_REPEAT_TIMEOUT, (time_t)JP_GIVUP_TIMEOUT ,&msg, &fname);
	if ( !ret ) { 
		readnew = !readnew;
		if ( readnew ) ret = glite_lbu_MaildirTransStart(sandbox_mdir, &msg, &fname);
		else ret = glite_lbu_MaildirRetryTransStart(sandbox_mdir, (time_t)JP_REPEAT_TIMEOUT, (time_t)JP_GIVUP_TIMEOUT ,&msg, &fname);
		if ( !ret ) {
			readnew = !readnew;
			return 0;
		}
	}

	if ( ret < 0 ) {
		dprintf("[%s] glite_lbu_MaildirTransStart: %s (%s)\n", name, strerror(errno), lbm_errdesc);
		if ( !debug ) syslog(LOG_ERR, "glite_lbu_MaildirTransStart: %s (%s)", strerror(errno), lbm_errdesc);
		return -1;
	}

	dprintf("[%s] sandbox JP import request received\n", name);
	if ( !debug ) syslog(LOG_INFO, "sandbox JP import request received");

	ret = 0;
	if ( parse_msg(msg, tab) < 0 ) {
		dprintf("[%s] Wrong format of message!\n", name);
		if ( !debug ) syslog(LOG_ERR, "Wrong format of message");
		ret = 0;
	} else do {
		su_in.job = tab[_job].val;
		// XXX: defined in org.glite.jp.primary/src/builtin_plugins.h
		// shloud use symbolic const...
		// do not distinquish between ibs and obs now 
		su_in.class_ = "urn:org.glite.jp.primary:isb";
		su_in.name = NULL;
		su_in.commitBefore = 1000 + time(NULL);
		su_in.contentType = "tar/lb";
		dprintf("[%s] Importing LB sandbox tar file '%s'\n", name, tab[_file].val);
		if ( !debug ) syslog(LOG_INFO, "Importing LB sandbox tar file '%s'\n", msg);
#ifdef JP_PERF
		if (!(sink & 2)) {
#endif
		refresh_connection(soap);
		ret = soap_call___jpsrv__StartUpload(soap, tab[_jpps].val?:jpps, "", &su_in, &su_out);
		ret = check_soap_fault(soap, ret);
		/* XXX: grrrrrrr! test it!!!*/
//		if ( (ret = check_soap_fault(soap, ret)) ) break;
		dprintf("[%s] Destination: %s\n\tCommit before: %s\n", name, su_out.destination, ctime(&su_out.commitBefore));

		if ( (fhnd = open(tab[_file].val, O_RDONLY)) < 0 ) {
			dprintf("[%s] Can't open sandbox tar file: %s\n", name, tab[_file].val);
			if ( !debug ) syslog(LOG_ERR, "Can't open sandbox tar file: %s", tab[_file].val);
			ret = 1;
			break;
		}
		if ( (ret = gftp_put_file(su_out.destination, fhnd)) ) break;
		close(fhnd);
		dprintf("[%s] File sent, commiting the upload\n", name);
		cu_in.destination = su_out.destination;
		refresh_connection(soap);
		ret = soap_call___jpsrv__CommitUpload(soap, tab[_jpps].val?:jpps, "", &cu_in, &empty);
		if ( (ret = check_soap_fault(soap, ret)) ) break;
		dprintf("[%s] Dump upload succesfull\n", name);
#ifdef JP_PERF
		} else ret = 0;
#endif
	} while (0);

	glite_lbu_MaildirTransEnd(sandbox_mdir, fname, ret? LBMD_TRANS_FAILED_RETRY: LBMD_TRANS_OK);
	free(fname);
	free(msg);

	return 1;
}


/** Parses every line looking for pattern string and stores the value into
 *  the given variable
 *
 *  line format is: key[space(s)]+val
 */
int parse_msg(char *msg, msg_pattern_t tab[])
{
	char	   *eol = msg,
			   *key, *val;
	
	while ( eol && *eol != '\0' ) {
		int		i;

		key = eol;
		if ( (eol = strchr(key, '\n')) ) *eol++ = '\0';
		while ( isblank(*key) ) key++;
		if ( *key == '\0' ) continue;
		val = key;
		while ( !isblank(*val) ) val++;
		if ( *val == '\0' ) return -1;
		*val++ = '\0';
		while ( isblank(*val) ) val++;
		if ( *val == '\0' ) return -1;

		for ( i = 0; tab[i].key; i++ ) {
			if ( !strcmp(tab[i].key, key) ) {
				tab[i].val = val;
				break;
			}
		}
	}

	return 0;
}


#define BUFSZ			1024

static globus_mutex_t	gLock;
static globus_cond_t	gCond;
static globus_bool_t	gDone;
static globus_bool_t	gError = GLOBUS_FALSE;
static globus_byte_t	gBuffer[BUFSZ];
static int				gOffset;


static void gftp_done_cb(
	void					   *user_arg,
	globus_ftp_client_handle_t *handle,
	globus_object_t			   *err)
{
	if ( err != GLOBUS_SUCCESS ) {
		char   *tmp = globus_object_printable_to_string(err);
		dprintf("[%s] Error in callback: %s\n", name, tmp);
		if ( !debug ) syslog(LOG_ERR, "Error in callback: %s", tmp);
		gError = GLOBUS_TRUE;
		globus_libc_free(tmp);
	}
	globus_mutex_lock(&gLock);
	gDone = GLOBUS_TRUE;
	globus_cond_signal(&gCond);
	globus_mutex_unlock(&gLock);
}

static void gftp_data_cb(  	
	void					   *user_arg,
	globus_ftp_client_handle_t *handle,
	globus_object_t			   *error,
	globus_byte_t			   *buffer,
	globus_size_t				length,
	globus_off_t				offset,
	globus_bool_t				eof)
{
	if ( !eof ) {
		int rc;
		globus_mutex_lock(&gLock);
		if ( (rc = read(*((int *)user_arg), gBuffer, BUFSZ)) < 0 ) {
			dprintf("[%s] Error reading dump file\n", name);
			if ( !debug ) syslog(LOG_ERR, "Error reading dump file");
			gDone = GLOBUS_TRUE;
			gError = GLOBUS_TRUE;
			globus_cond_signal(&gCond);
		} else {
			globus_ftp_client_register_write(
					handle, gBuffer, rc, gOffset, rc == 0, gftp_data_cb, user_arg);
			gOffset += rc;
		}
		globus_mutex_unlock(&gLock);
	}
}

static int gftp_put_file(const char *url, int fhnd)
{
	static globus_ftp_client_operationattr_t	op_attr;
	static globus_ftp_client_handleattr_t		hnd_attr;
	int gftp_retried = 0;

	globus_mutex_init(&gLock, GLOBUS_NULL);
	globus_cond_init(&gCond, GLOBUS_NULL);

	/* one lost connection survival cycle */
	do {

	if (!gftp_initialized++) {
#define put_file_err(errs)		{			\
	dprintf("[%s] %s\n", name, errs);		\
	if ( !debug ) syslog(LOG_ERR, errs);	\
	return 1;								\
}
	if ( globus_ftp_client_handleattr_init(&hnd_attr) != GLOBUS_SUCCESS )
		put_file_err("Could not initialise handle attributes");
	
	if ( globus_ftp_client_handleattr_set_cache_all(&hnd_attr, GLOBUS_TRUE) != GLOBUS_SUCCESS)
		put_file_err("Could not set connection caching");

	if ( globus_ftp_client_operationattr_init(&op_attr) != GLOBUS_SUCCESS )
		put_file_err("Could not initialise operation attributes");
	
	if ( globus_ftp_client_handle_init(&hnd, &hnd_attr) != GLOBUS_SUCCESS )
		put_file_err("Could not initialise ftp client handle");
	}
	if ( globus_ftp_client_operationattr_set_authorization(
			&op_attr, server_cert? mycred->gss_cred: GSS_C_NO_CREDENTIAL,
			NULL, "", 0, NULL) != GLOBUS_SUCCESS )
		put_file_err("Could not set authorization procedure");
#undef put_file_err

	gDone = GLOBUS_FALSE;
	gError = GLOBUS_FALSE;

	/* do the op */
	if ( globus_ftp_client_put(
				&hnd, url, &op_attr,
				GLOBUS_NULL, gftp_done_cb, (void *)&fhnd) != GLOBUS_SUCCESS) {
		dprintf("[%s] Could not start file put\n", name);
		if ( !debug ) syslog(LOG_ERR, "Could not start file put");
		gError = GLOBUS_TRUE;
		gDone = GLOBUS_TRUE;
	} else {
		int rc;
		globus_mutex_lock(&gLock);
		if ( (rc = read(fhnd, gBuffer, BUFSZ)) < 0 ) {
			dprintf("[%s] Error reading dump file\n", name);
			if ( !debug ) syslog(LOG_ERR, "Error reading dump file");
			gDone = GLOBUS_TRUE;
			gError = GLOBUS_TRUE;
			globus_cond_signal(&gCond);
		} else {
			globus_ftp_client_register_write(&hnd,
					gBuffer, rc, gOffset, rc == 0, gftp_data_cb, (void *)&fhnd);
			gOffset += rc;
		}
		globus_mutex_unlock(&gLock);
	}

	globus_mutex_lock(&gLock);
	while ( !gDone ) globus_cond_wait(&gCond, &gLock);
	globus_mutex_unlock(&gLock);

        if (gError == GLOBUS_TRUE) {
        	gftp_retried++;
		gftp_initialized = 0;
		globus_ftp_client_handle_destroy(&hnd);
		dprintf("[%s] %s: FTP upload failed\n", name, gftp_retried <= 1 ? "Warning" : "Error");
        }
    } while (gError == GLOBUS_TRUE && gftp_retried <= 1);

    return (gError == GLOBUS_TRUE)? 1: 0;
}


static int refresh_connection(struct soap *soap) {
	struct timeval		to = {JPPS_NO_RESPONSE_TIMEOUT, 0};
	edg_wll_GssCred newcred;
	edg_wll_GssStatus	gss_code;
	glite_gsplugin_Context gp_ctx;

	gp_ctx = glite_gsplugin_get_context(soap);
	glite_gsplugin_set_timeout(gp_ctx, &to);

	switch ( edg_wll_gss_watch_creds(server_cert, &cert_mtime) ) {
	case 0: break;
	case 1:
		if ( !edg_wll_gss_acquire_cred_gsi(server_cert, server_key, &newcred, &gss_code) ) {
			dprintf("[%s] reloading credentials successful\n", name);
			edg_wll_gss_release_cred(&mycred, &gss_code);
			mycred = newcred;
			glite_gsplugin_set_credential(gp_ctx, newcred);
		} else { dprintf("[%s] reloading credentials failed, using old ones\n", name); }
		break;
	case -1: dprintf("[%s] edg_wll_gss_watch_creds failed\n", name); break;
	}

	return 0;
}


#ifdef JP_PERF
static void stats_init(perf_t *perf, const char *name) {
	struct timeval tv;

	memset(perf, 0, sizeof *perf);
	perf->count = 0;
	perf->name = strdup(name);
	gettimeofday(&tv, NULL);
	perf->start = tv.tv_sec + (double)tv.tv_usec / 1000000.0;
	dprintf("[%s statistics] start detected\n", name);
}

static void stats_set_jobid(perf_t *perf, const char *jobid) {
	perf->id = strdup(jobid + sizeof(PERF_JOBID_START_PREFIX) - 1);
	dprintf("[%s statistics] ID %s\n", perf->name, perf->id);
}

static void stats_get_limit(perf_t *perf, const char *name) {
	FILE *f;
	char *fn, item[200];
	int count;

	/* stopper */
	asprintf(&fn, PERF_STOP_FILE_FORMAT, perf->id);
	f = fopen(fn, "rt");
	free(fn);
	if (f) {
		fscanf(f, "%s\t%d", item, &count);
		if (strcasecmp(item, name) != 0) fscanf(f, "%s\t%d", item, &count);
		dprintf("[%s statistics] expected %d %s\n", name, count, item);
		fclose(f);
		perf->limit = count;
	}
}

static void stats_done(perf_t *perf) {
	struct timeval tv;

	gettimeofday(&tv, NULL);
	perf->end = tv.tv_sec + (double)tv.tv_usec / 1000000.0;
	dprintf("[%s statistics] %s\n", perf->name, perf->id);
	dprintf("[%s statistics] start: %lf\n", perf->name, perf->start);
	dprintf("[%s statistics] stop:  %lf\n", perf->name, perf->end);
	dprintf("[%s statistics] count: %ld (%lf job/day)\n", perf->name, perf->count, 86400.0 * perf->count / (perf->end - perf->start));
	free(perf->id);
	free(perf->name);
	memset(perf, 0, sizeof *perf);
}
#endif

/* XXX: we don't use it */
SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };

