#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
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

#ifndef GLITE_JPIMPORTER_MDIR
#define GLITE_JPIMPORTER_MDIR		"/tmp/jpreg"
#endif 

static int	debug = 0;
static int	die = 0;

static struct option opts[] = {
	{ "help",        0, NULL,    'h'},
	{ "debug",       0, NULL,    'd'},
	{ "jpps",        1, NULL,    'p'},
	{ "mdir",        1, NULL,    'm'},
	{ "pidfile",     1, NULL,    'i'},
	{ NULL,          0, NULL,     0}
};

static const char *get_opt_string = "hdp:m:i:";

static void usage(char *me)
{
	fprintf(stderr,"usage: %s [option]\n"
		"\t-h, --help\t displays this screen\n"
		"\t-d, --debug\t don't run as daemon, additional diagnostics\n"
		"\t-p, --jpps\t JP primary service server\n"
		"\t-m, --mdir\t path to the 'LB maildir' subtree\n"
		"\t-i, --pidfile\t file to store master pid\n",
		me);
}

static void catchsig(int sig)
{
	die = sig;
}

int main(int argc, char *argv[])
{
	struct sigaction	sa;
	struct soap		   *soap;
	sigset_t			sset;
	FILE			   *fpid;
	int					opt;
	char			   *name,
					   *jpps = "http://localhost:8901",
						pidfile[PATH_MAX] = GLITE_JPIMPORTER_PIDFILE,
						mdir[PATH_MAX] = GLITE_JPIMPORTER_MDIR;


	name = strrchr(argv[0],'/');
	if (name) name++; else name = argv[0];

	if ( geteuid() )
		snprintf(pidfile, sizeof pidfile, "%s/glite_jpimporter.pid", getenv("HOME"));

	while ( (opt = getopt_long(argc, argv, get_opt_string, opts, NULL)) != EOF )
	switch ( opt ) {
		case 'd': debug = 1; break;
		case 'h': usage(name); return 0;
		case 'p': jpps = optarg; break;
		case 'm': strcpy(mdir, optarg); break;
		case 'i': strcpy(pidfile, optarg); break;
		case '?': usage(name); return 1;
	}
	if ( optind < argc ) { usage(name); return 1; }

	soap = soap_new();
	soap_init(soap);
	soap_set_namespaces(soap, jpps__namespaces);
	soap_register_plugin(soap, glite_gsplugin);

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

	sa.sa_handler = SIG_IGN;
	sigaction(SIGUSR1, &sa, NULL);

	sigemptyset(&sset);
	sigaddset(&sset, SIGTERM);
	sigaddset(&sset, SIGINT);
	sigprocmask(SIG_BLOCK, &sset, NULL);

	while ( !die ) {
		int		ret;
		char   *msg = NULL;
		char   *fname = NULL;

		ret = edg_wll_MaildirTransStart(mdir, &msg, &fname);
		/* XXX: where should unblocking signal besides? */
		sigprocmask(SIG_UNBLOCK, &sset, NULL);
		sigprocmask(SIG_BLOCK, &sset, NULL);
		if ( ret < 0 ) {
			dprintf(("edg_wll_MaildirTransStart: %s (%s)\n", strerror(errno), lbm_errdesc));
			if ( !debug ) syslog(LOG_ERR, "edg_wll_MaildirTransStart: %s (%s)", strerror(errno), lbm_errdesc);
			exit(1);
		} else if ( ret == 0 ) {
			sleep(2);
		} else {
			struct _jpelem__RegisterJob			in;
			struct _jpelem__RegisterJobResponse	empty;
			struct SOAP_ENV__Detail				*detail;
			struct jptype__genericFault			*f;
			char	*aux, *reason, indent[200] = "  ";


			dprintf(("JP registration request received\n"));
			if ( !debug ) syslog(LOG_INFO, "JP registration request received\n");

			if ( !(aux = strchr(msg, '\n')) ) {
				dprintf(("Wrong format of message!\n"));
				if ( !debug ) syslog(LOG_ERR, "Wrong format of message\n");
				free(msg);
				continue;
			}
			*aux++ = '\0';
			in.job = msg;
			in.owner = aux;
			ret = soap_call___jpsrv__RegisterJob(soap, jpps, "", &in, &empty);
			free(msg);

			switch ( ret ) {
			case SOAP_OK:
				/* XXX: checks return error code */
				edg_wll_MaildirTransEnd(mdir, fname, LBMD_TRANS_OK);
				dprintf(("Job '%s' succesfully registered to JP\n", msg));
				if ( !debug ) syslog(LOG_INFO, "Job '%s' succesfully registered to JP\n", msg);
				break;

			case SOAP_FAULT:
			case SOAP_SVR_FAULT:
				edg_wll_MaildirTransEnd(mdir, fname, LBMD_TRANS_FAILED);
				if (soap->version == 2) {
					detail = soap->fault->SOAP_ENV__Detail;
					reason = soap->fault->SOAP_ENV__Reason;
				} else {
					detail = soap->fault->detail;
					reason = soap->fault->faultstring;
				}
				dprintf(("%s\n", reason));
				assert(detail->__type == SOAP_TYPE__genericFault);
#if GSOAP_VERSION >=20700
				f = ((struct _genericFault *) detail->fault)
#else
				f = ((struct _genericFault *) detail->value)
#endif
					-> jpelem__genericFault;

				while ( f ) {
					dprintf(("%s%s: %s (%s)\n", indent, f->source, f->text, f->description));
					f = f->reason;
					strcat(indent, "  ");
				}
				break;

			default:
				soap_print_fault(soap, stderr);
				edg_wll_MaildirTransEnd(mdir, fname, LBMD_TRANS_FAILED);
				break;
			}
			free(fname);
		}
	}

	/* XXX: some sort of soap_destroy(soap) */
	dprintf(("Terminating on signal %d\n", die));
	if ( !debug ) syslog(LOG_INFO, "Terminating on signal %d\n", die);

	unlink(pidfile);

	return 0;
}

/* XXX: we don't use it */
SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };

