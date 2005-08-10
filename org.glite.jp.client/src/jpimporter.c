#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <syslog.h>
#include <linux/limits.h>
#include <fcntl.h>

#include "glite/lb/lb_maildir.h"
#include "glite/security/glite_gsplugin.h"

#include "jpps_H.h"
#include "jpps_.nsmap"

#include "jptype_map.h"

#include "soap_version.h"
#if GSOAP_VERSION <= 20602
#define soap_call___jpsrv__RegisterJob soap_call___ns1__RegisterJob
#endif


#ifndef dprintf
#define dprintf(x)		{ if (debug) printf x; }
#endif

#ifndef GLITE_JPIMPORTER_PIDFILE
#define GLITE_JPIMPORTER_PIDFILE	"/var/run/glite-jpimporter.pid"
#endif 

#ifndef GLITE_REG_IMPORTER_MDIR
#define GLITE_REG_IMPORTER_MDIR		"/tmp/jpreg"
#endif 

#ifndef GLITE_DUMP_IMPORTER_MDIR
#define GLITE_DUMP_IMPORTER_MDIR	"/tmp/jpdump"
#endif 

#ifndef GLITE_JPPS
#define GLITE_JPPS					"http://localhost:8901"
#endif 

#define	MAX_REG_CONNS				500

static int			debug = 0;
static int			die = 0;
static int			child_died = 0;
static int			poll = 2;
static char		   *jpps = GLITE_JPPS;
static char			reg_mdir[PATH_MAX] = GLITE_REG_IMPORTER_MDIR;
static char			dump_mdir[PATH_MAX] = GLITE_DUMP_IMPORTER_MDIR;
static struct soap *soap;

static struct option opts[] = {
	{ "help",        0, NULL,    'h'},
	{ "debug",       0, NULL,    'g'},
	{ "jpps",        1, NULL,    'p'},
	{ "reg-mdir",    1, NULL,    'r'},
	{ "dump-mdir",   1, NULL,    'd'},
	{ "pidfile",     1, NULL,    'i'},
	{ "poll",        1, NULL,    't'},
	{ NULL,          0, NULL,     0}
};

static const char *get_opt_string = "hgp:r:d::i:t:";

static void usage(char *me)
{
	fprintf(stderr,"usage: %s [option]\n"
		"\t-h, --help         displays this screen\n"
		"\t-g, --debug        don't run as daemon, additional diagnostics\n"
		"\t-p, --jpps         JP primary service server\n"
		"\t-r, --reg-mdir     path to the 'LB maildir' subtree for registrations\n"
		"\t-d, --dump-mdir    path to the 'LB maildir' subtree for LB dumps\n"
		"\t-i, --pidfile      file to store master pid\n"
		"\t-t, --poll         maildir polling interval (in seconds)\n",
		me);
}

static void catchsig(int sig)
{
	die = sig;
}

static void catch_chld(int sig)
{
	child_died = 1;
}


static int slave(int (*)(const char *), const char *);
static int check_soap_fault(struct soap *, int, const char *);
static int reg_importer(const char *);
static int dump_importer(const char *);



int main(int argc, char *argv[])
{
	struct sigaction	sa;
	sigset_t			sset;
	FILE			   *fpid;
	pid_t				reg_pid, dump_pid;
	int					opt;
	char			   *name,
						pidfile[PATH_MAX] = GLITE_JPIMPORTER_PIDFILE;


	name = strrchr(argv[0],'/');
	if (name) name++; else name = argv[0];

	if ( geteuid() )
		snprintf(pidfile, sizeof pidfile, "%s/glite_jpimporter.pid", getenv("HOME"));

	while ( (opt = getopt_long(argc, argv, get_opt_string, opts, NULL)) != EOF )
	switch ( opt ) {
		case 'g': debug = 1; break;
		case 'h': usage(name); return 0;
		case 'p': jpps = optarg; break;
		case 't': poll = atoi(optarg); break;
		case 'r': strcpy(reg_mdir, optarg); break;
		case 'd': strcpy(dump_mdir, optarg); break;
		case 'i': strcpy(pidfile, optarg); break;
		case '?': usage(name); return 1;
	}
	if ( optind < argc ) { usage(name); return 1; }

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
		
	if ( !debug ) {
		if ( daemon(1,0) == -1 ) { perror("deamon()"); exit(1); }

		fpid = fopen(pidfile,"w");
		if ( !fpid ) { perror(pidfile); return 1; }
		fprintf(fpid, "%d", getpid());
		fclose(fpid);
		openlog(name, LOG_PID, LOG_DAEMON);
	} else { setpgid(0, getpid()); }

	dprintf(("Master pid %d\n", getpid()));

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

	soap = soap_new();
	soap_init(soap);
	soap_set_namespaces(soap, jpps__namespaces);
	soap_register_plugin(soap, glite_gsplugin);

	if ( (reg_pid = slave(reg_importer, "reg-imp")) < 0 ) {
		perror("starting reg importer slave");
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
						dprintf(("[master] reg importer slave died [%d]\n", pid));
						if (!debug) syslog(LOG_INFO, "reg importer slave died [%d]\n", die);
						if ( (reg_pid = slave(reg_importer, "reg-imp")) < 0 ) {
							perror("starting reg importer slave");
							kill(0, SIGINT);
							exit(1);
						}
						dprintf(("[master] reg importer slave restarted [%d]\n", reg_pid));
					}
				}
			}
			child_died = 0;
			continue;
		}
	}

	dprintf(("[master] Terminating on signal %d\n", die));
	if (!debug) syslog(LOG_INFO, "Terminating on signal %d\n", die);
	kill(0, die);

	unlink(pidfile);

	return 0;
}

static int slave(int (*fn)(const char *), const char *nm)
{
	struct sigaction	sa;
	sigset_t			sset;
	int					pid,
						conn_cnt = 0;


	if ( (pid = fork()) ) return pid;

	memset(&sa, 0, sizeof(sa)); assert(sa.sa_handler == NULL);
	sa.sa_handler = catchsig;
	sigaction(SIGUSR1, &sa, NULL);

	sigemptyset(&sset);
	sigaddset(&sset, SIGTERM);
	sigaddset(&sset, SIGINT);
	sigaddset(&sset, SIGUSR1);
	sigprocmask(SIG_BLOCK, &sset, NULL);

	dprintf(("[%s] slave started - pid [%d]\n", nm, getpid()));

	while ( !die && conn_cnt < MAX_REG_CONNS ) {
		int ret = fn(nm);

		if ( ret > 0 ) conn_cnt++;
		else if ( ret < 0 ) exit(1);
		else if ( ret == 0 ) {
			sigprocmask(SIG_UNBLOCK, &sset, NULL);
			sleep(poll);
			sigprocmask(SIG_BLOCK, &sset, NULL);
		} 
	}

	if ( die ) {
		dprintf(("[%s] Terminating on signal %d\n", nm, getpid(), die));
		if ( !debug ) syslog(LOG_INFO, "Terminating on signal %d", die);
	}
    dprintf(("[%s] Terminating after %d connections\n", nm, conn_cnt));
    if ( !debug ) syslog(LOG_INFO, "Terminating after %d connections", conn_cnt);

	exit(0);
}


static int reg_importer(const char *nm)
{
	struct _jpelem__RegisterJob			in;
	struct _jpelem__RegisterJobResponse	empty;
	int			ret;
	char	   *msg = NULL,
			   *fname = NULL,
			   *aux;


	ret = edg_wll_MaildirTransStart(reg_mdir, &msg, &fname);
	if ( ret < 0 ) {
		dprintf(("[%s] edg_wll_MaildirTransStart: %s (%s)\n", nm, strerror(errno), lbm_errdesc));
		if ( !debug ) syslog(LOG_ERR, "edg_wll_MaildirTransStart: %s (%s)", strerror(errno), lbm_errdesc);
		return -1;
	} else if ( ret > 0 ) {
		dprintf(("[%s] JP registration request received\n", nm));
		if ( !debug ) syslog(LOG_INFO, "JP registration request received\n");

		ret = 0;
		if ( !(aux = strchr(msg, '\n')) ) {
			dprintf(("[%s] Wrong format of message!\n", nm));
			if ( !debug ) syslog(LOG_ERR, "Wrong format of message\n");
			ret = 0;
		} else {
			*aux++ = '\0';
			in.job = msg;
			in.owner = aux;
			dprintf(("[%s] Registering '%s'\n", nm, msg));
			if ( !debug ) syslog(LOG_INFO, "Registering '%s'\n", msg);
			ret = soap_call___jpsrv__RegisterJob(soap, jpps, "", &in, &empty);
			ret = check_soap_fault(soap, ret, nm);
		}
		edg_wll_MaildirTransEnd(reg_mdir, fname, ret? LBMD_TRANS_FAILED: LBMD_TRANS_OK);
		free(fname);
		free(msg);
		return 1;
	}

	return 0;
}

static int dump_importer(const char *nm)
{
	struct _jpelem__StartUpload			in;
	struct _jpelem__StartUploadResponse	out;
	int			ret;
	char	   *msg = NULL,
			   *fname = NULL,
			   *aux;


	ret = edg_wll_MaildirTransStart(dump_mdir, &msg, &fname);
	if ( ret < 0 ) {
		dprintf(("[%s] edg_wll_MaildirTransStart: %s (%s)\n", nm, strerror(errno), lbm_errdesc));
		if ( !debug ) syslog(LOG_ERR, "edg_wll_MaildirTransStart: %s (%s)", strerror(errno), lbm_errdesc);
		return -1;
	} else if ( ret > 0 ) {
		dprintf(("[%s] dump JP import request received\n", nm));
		if ( !debug ) syslog(LOG_INFO, "dump JP import request received\n");

		ret = 0;
		if ( !(aux = strchr(msg, '\n')) ) {
			dprintf(("[%s] Wrong format of message!\n", nm));
			if ( !debug ) syslog(LOG_ERR, "Wrong format of message\n");
			ret = 0;
		} else {
			*aux++ = '\0';
			in.job = argv[2];
			in.class_ = argv[3];
			in.name = NULL;
			in.commitBefore = atoi(argv[4]) + time(NULL);
			in.contentType = argv[5];
			dprintf(("[%s] Importing LB dump file '%s'\n", nm, msg));
			if ( !debug ) syslog(LOG_INFO, "Importing LB dump file '%s'\n", msg);
			ret = soap_call___jpsrv__StartUpload(soap, jpps, "", &in, &out);
			ret = check_soap_fault(soap, ret, nm);
		}
		edg_wll_MaildirTransEnd(dump_mdir, fname, ret? LBMD_TRANS_FAILED: LBMD_TRANS_OK);
		free(fname);
		free(msg);
		return 1;
	}

	return 0;
}


static int check_soap_fault(struct soap *soap, int err, const char *msg_pref)
{
	struct SOAP_ENV__Detail		   *detail;
	struct jptype__genericFault	   *f;
	char						   *reason,
									indent[200] = "  ";
		

	switch ( err ) {
	case SOAP_OK:
		dprintf(("[%s] ok\n", msg_pref));
		break; 

	case SOAP_FAULT:
	case SOAP_SVR_FAULT:
		if (soap->version == 2) {
			detail = soap->fault->SOAP_ENV__Detail;
			reason = soap->fault->SOAP_ENV__Reason;
		} else {
			detail = soap->fault->detail;
			reason = soap->fault->faultstring;
		}
		dprintf(("[%s] %s\n", msg_pref, reason));
		if ( !debug ) syslog(LOG_ERR, "%s %s", msg_pref, reason);
		assert(detail->__type == SOAP_TYPE__genericFault);
#if GSOAP_VERSION >=20700
		f = ((struct _genericFault *) detail->fault) -> jpelem__genericFault;
#else
		f = ((struct _genericFault *) detail->value) -> jpelem__genericFault;
#endif
		while (f) {
			dprintf(("[%s] %s%s: %s (%s)\n",
					msg_pref, indent,
					f->source, f->text, f->description));
			if ( !debug ) syslog(LOG_ERR, "%s %s%s: %s (%s)",
					msg_pref, reason,
					f->source, f->text, f->description);
			f = f->reason;
			strcat(indent, "  ");
		}
		return -1;

	default: soap_print_fault(soap,stderr);
		return -1;
	}

	return 0;
}

/* XXX: we don't use it */
SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };

