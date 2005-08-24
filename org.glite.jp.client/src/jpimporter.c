#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>
#include <fcntl.h>

#include "glite/lb/lb_maildir.h"
#include "glite/security/glite_gsplugin.h"

#include "jpps_H.h"
#include "jpps_.nsmap"

#include "jptype_map.h"

#include "globus_ftp_client.h"

#include "soap_version.h"
#if GSOAP_VERSION <= 20602
#define soap_call___jpsrv__RegisterJob soap_call___ns1__RegisterJob
#endif


typedef struct {
	char	   *key;
	char	   *val;
} msg_pattern_t;


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

static int				debug = 0;
static int				die = 0;
static int				child_died = 0;
static int				poll = 2;
static char			   *name;
static char			   *jpps = GLITE_JPPS;
static char				reg_mdir[PATH_MAX] = GLITE_REG_IMPORTER_MDIR;
static char				dump_mdir[PATH_MAX] = GLITE_DUMP_IMPORTER_MDIR;
static struct soap	   *soap;

static time_t			cert_mtime;
static char			   *server_cert = NULL,
					   *server_key = NULL,
					   *cadir;
static gss_cred_id_t	mycred = GSS_C_NO_CREDENTIAL;
static char			   *mysubj;


static struct option opts[] = {
	{ "help",        0, NULL,    'h'},
	{ "cert",        1, NULL,    'c'},
	{ "key",         1, NULL,    'k'},
	{ "CAdir",       1, NULL,    'C'},
	{ "debug",       0, NULL,    'g'},
	{ "jpps",        1, NULL,    'p'},
	{ "reg-mdir",    1, NULL,    'r'},
	{ "dump-mdir",   1, NULL,    'd'},
	{ "pidfile",     1, NULL,    'i'},
	{ "poll",        1, NULL,    't'},
	{ NULL,          0, NULL,     0}
};

static const char *get_opt_string = "hgp:r:d::i:t:c:k:C:";

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


static int slave(int (*)(void), const char *);
static int check_soap_fault(struct soap *, int);
static int reg_importer(void);
static int dump_importer(void);
static int parse_msg(char *, msg_pattern_t []);
static int gftp_put_file(const char *, int);



int main(int argc, char *argv[])
{
	edg_wll_GssStatus	gss_code;
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
		case 'c': server_cert = optarg; break;
		case 'k': server_key = optarg; break;
		case 'C': cadir = optarg; break;
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

	if ( globus_module_activate(GLOBUS_FTP_CLIENT_MODULE) != GLOBUS_SUCCESS ) {
		dprintf(("[master] Could not activate ftp client module\n"));
		if (!debug) syslog(LOG_INFO, "Could not activate ftp client module\n");
		exit(1);
	} else dprintf(("[master] Ftp client module activated\n"));
	
	if ( !server_cert || !server_key )
		fprintf(stderr, "%s: key or certificate file not specified"
						" - unable to watch them for changes!\n", argv[0]);
	if ( cadir ) setenv("X509_CERT_DIR", cadir, 1);
	edg_wll_gss_watch_creds(server_cert, &cert_mtime);
	if ( !edg_wll_gss_acquire_cred_gsi(server_cert, server_key, &mycred, &mysubj, &gss_code) ) {
		dprintf(("[master] Server identity: %s\n", mysubj));
	} else {
		char *errmsg;
		edg_wll_gss_get_error(&gss_code, "edg_wll_gss_acquire_cred_gsi()", &errmsg);
		dprintf(("[master] %s\n", errmsg));
		free(errmsg);
		dprintf(("[master] Running unauthenticated\n"));
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

	soap = soap_new();
	soap_init(soap);
	soap_set_namespaces(soap, jpps__namespaces);
	soap_register_plugin(soap, glite_gsplugin);

	if ( (reg_pid = slave(reg_importer, "reg-imp")) < 0 ) {
		perror("starting reg importer slave");
		exit(1);
	}
	if ( (dump_pid = slave(dump_importer, "dump-imp")) < 0 ) {
		perror("starting dump importer slave");
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
					} else if ( pid == dump_pid ) {
						dprintf(("[master] dump importer slave died [%d]\n", pid));
						if (!debug) syslog(LOG_INFO, "dump importer slave died [%d]\n", die);
						if ( (dump_pid = slave(dump_importer, "dump-imp")) < 0 ) {
							perror("starting dump importer slave");
							kill(0, SIGINT);
							exit(1);
						}
						dprintf(("[master] dump importer slave restarted [%d]\n", dump_pid));
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

    globus_module_deactivate_all();
	unlink(pidfile);

	return 0;
}

static int slave(int (*fn)(void), const char *nm)
{
	struct sigaction	sa;
	sigset_t			sset;
	int					pid,
						conn_cnt = 0;


	if ( (pid = fork()) ) return pid;

	name = (char *)nm;
	memset(&sa, 0, sizeof(sa)); assert(sa.sa_handler == NULL);
	sa.sa_handler = catchsig;
	sigaction(SIGUSR1, &sa, NULL);

	sigemptyset(&sset);
	sigaddset(&sset, SIGTERM);
	sigaddset(&sset, SIGINT);
	sigaddset(&sset, SIGUSR1);
	sigprocmask(SIG_BLOCK, &sset, NULL);

	dprintf(("[%s] slave started - pid [%d]\n", name, getpid()));

	while ( !die && conn_cnt < MAX_REG_CONNS ) {
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
		dprintf(("[%s] Terminating on signal %d\n", name, getpid(), die));
		if ( !debug ) syslog(LOG_INFO, "Terminating on signal %d", die);
	}
    dprintf(("[%s] Terminating after %d connections\n", name, conn_cnt));
    if ( !debug ) syslog(LOG_INFO, "Terminating after %d connections", conn_cnt);

	exit(0);
}


static int reg_importer(void)
{
	struct _jpelem__RegisterJob			in;
	struct _jpelem__RegisterJobResponse	empty;
	int			ret;
	char	   *msg = NULL,
			   *fname = NULL,
			   *aux;


	ret = edg_wll_MaildirTransStart(reg_mdir, &msg, &fname);
	if ( ret < 0 ) {
		dprintf(("[%s] edg_wll_MaildirTransStart: %s (%s)\n", name, strerror(errno), lbm_errdesc));
		if ( !debug ) syslog(LOG_ERR, "edg_wll_MaildirTransStart: %s (%s)", strerror(errno), lbm_errdesc);
		return -1;
	} else if ( ret > 0 ) {
		dprintf(("[%s] JP registration request received\n", name));
		if ( !debug ) syslog(LOG_INFO, "JP registration request received\n");

		ret = 0;
		if ( !(aux = strchr(msg, '\n')) ) {
			dprintf(("[%s] Wrong format of message!\n", name));
			if ( !debug ) syslog(LOG_ERR, "Wrong format of message\n");
			ret = 0;
		} else do {
			*aux++ = '\0';
			in.job = msg;
			in.owner = aux;
			dprintf(("[%s] Registering '%s'\n", name, msg));
			if ( !debug ) syslog(LOG_INFO, "Registering '%s'\n", msg);
			ret = soap_call___jpsrv__RegisterJob(soap, jpps, "", &in, &empty);
			if ( (ret = check_soap_fault(soap, ret)) ) break;
		} while (0);
		edg_wll_MaildirTransEnd(reg_mdir, fname, ret? LBMD_TRANS_FAILED: LBMD_TRANS_OK);
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
	static int		readnew = 1;
	char		   *msg = NULL,
				   *fname = NULL,
				   *aux;
	int				ret;
	int				fhnd;
	msg_pattern_t	tab[] = {
						{"jobid", NULL},
						{"file", NULL},
						{"jpps", NULL},
						{NULL, NULL}};
#define				_job  0
#define				_file 1
#define				_jpps 2


	if ( readnew ) ret = edg_wll_MaildirTransStart(dump_mdir, &msg, &fname);
	else ret = edg_wll_MaildirRetryTransStart(dump_mdir, (time_t)60, &msg, &fname);
	if ( !ret ) { 
		readnew = ~readnew;
		if ( readnew ) ret = edg_wll_MaildirTransStart(dump_mdir, &msg, &fname);
		else ret = edg_wll_MaildirRetryTransStart(dump_mdir, (time_t)60, &msg, &fname);
		if ( !ret ) {
			readnew = ~readnew;
			return 0;
		}
	}

	if ( ret < 0 ) {
		dprintf(("[%s] edg_wll_MaildirTransStart: %s (%s)\n", name, strerror(errno), lbm_errdesc));
		if ( !debug ) syslog(LOG_ERR, "edg_wll_MaildirTransStart: %s (%s)", strerror(errno), lbm_errdesc);
		return -1;
	}

	dprintf(("[%s] dump JP import request received\n", name));
	if ( !debug ) syslog(LOG_INFO, "dump JP import request received");

	ret = 0;
	if ( parse_msg(msg, tab) < 0 ) {
		dprintf(("[%s] Wrong format of message!\n", name));
		if ( !debug ) syslog(LOG_ERR, "Wrong format of message");
		ret = 0;
	} else do {
		su_in.job = tab[_job].val;
		su_in.class_ = "urn:org.glite.jp.primary:lb";
		su_in.name = tab[_file].val;
		su_in.commitBefore = 1000 + time(NULL);
		su_in.contentType = "text/lb";
		dprintf(("[%s] Importing LB dump file '%s'\n", name, tab[_file].val));
		if ( !debug ) syslog(LOG_INFO, "Importing LB dump file '%s'\n", msg);
		ret = soap_call___jpsrv__StartUpload(soap, tab[_jpps].val?:jpps, "", &su_in, &su_out);
		ret = check_soap_fault(soap, ret);
		/* XXX: grrrrrrr! test it!!!
		if ( (ret = check_soap_fault(soap, ret)) ) break;
		dprintf(("[%s] Destination: %s\n\tCommit before: %s\n", su_out.destination, ctime(&su_out.commitBefore)));
		*/

		if ( (fhnd = open(tab[_file].val, O_RDONLY)) < 0 ) {
			dprintf(("[%s] Can't open dump file: %s\n", name, tab[_file].val));
			if ( !debug ) syslog(LOG_ERR, "Can't open dump file: %s", tab[_file].val);
			ret = 1;
			break;
		}
		/* XXX: grrrrrrr! remove next line!!! */
		su_out.destination = "gsiftp://nain.ics.muni.cz:5678/tmp/gsiftp-dump-tst-file";
		if ( (ret = gftp_put_file(su_out.destination, fhnd)) ) break;
		close(fhnd);
		dprintf(("[%s] File sent, commiting the upload\n", name));
		cu_in.destination = su_out.destination;
		ret = soap_call___jpsrv__CommitUpload(soap, tab[_jpps].val?:jpps, "", &cu_in, &empty);
		if ( (ret = check_soap_fault(soap, ret)) ) break;
		dprintf(("[%s] Dump upload succesfull\n", name));
	} while (0);

	edg_wll_MaildirTransEnd(dump_mdir, fname, ret? LBMD_TRANS_FAILED_RETRY: LBMD_TRANS_OK);
	free(fname);
	free(msg);

	return 1;
}


static int check_soap_fault(struct soap *soap, int err)
{
	struct SOAP_ENV__Detail		   *detail;
	struct jptype__genericFault	   *f;
	char						   *reason,
									indent[200] = "  ";
		

	switch ( err ) {
	case SOAP_OK:
		dprintf(("[%s] ok\n", name));
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
		dprintf(("[%s] %s\n", name, reason));
		if ( !debug ) syslog(LOG_ERR, "%s", reason);
		assert(detail->__type == SOAP_TYPE__genericFault);
#if GSOAP_VERSION >=20700
		f = ((struct _genericFault *) detail->fault) -> jpelem__genericFault;
#else
		f = ((struct _genericFault *) detail->value) -> jpelem__genericFault;
#endif
		while (f) {
			dprintf(("[%s] %s%s: %s (%s)\n",
					name, indent,
					f->source, f->text, f->description));
			if ( !debug ) syslog(LOG_ERR, "%s%s: %s (%s)",
					reason, f->source, f->text, f->description);
			f = f->reason;
			strcat(indent, "  ");
		}
		return -1;

	default: soap_print_fault(soap,stderr);
		return -1;
	}

	return 0;
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
		dprintf(("[%s] Error in callback: %s\n", name, tmp));
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
			dprintf(("[%s] Error reading dump file\n", name));
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
	globus_ftp_client_handle_t			hnd;
	globus_ftp_client_operationattr_t	op_attr;
	globus_ftp_client_handleattr_t		hnd_attr;

#define put_file_err(errs)		{			\
	dprintf(("[%s] %s\n", name, errs));		\
	if ( !debug ) syslog(LOG_ERR, errs);	\
	return 1;								\
}
	if ( globus_ftp_client_handleattr_init(&hnd_attr) != GLOBUS_SUCCESS )
		put_file_err("Could not initialise handle attributes");
	
	if ( globus_ftp_client_operationattr_init(&op_attr) != GLOBUS_SUCCESS )
		put_file_err("Could not initialise operation attributes");
	
	if ( globus_ftp_client_operationattr_set_authorization(
			&op_attr, server_cert? mycred: GSS_C_NO_CREDENTIAL,
			NULL, "", 0, NULL) != GLOBUS_SUCCESS )
		put_file_err("Could not set authorization procedure");

	if ( globus_ftp_client_handle_init(&hnd, &hnd_attr) != GLOBUS_SUCCESS )
		put_file_err("Could not initialise ftp client handle");
#undef put_file_err

	globus_mutex_init(&gLock, GLOBUS_NULL);
	globus_cond_init(&gCond, GLOBUS_NULL);

	gDone = GLOBUS_FALSE;

	/* do the op */
	if ( globus_ftp_client_put(
				&hnd, url, &op_attr,
				GLOBUS_NULL, gftp_done_cb, (void *)&fhnd) != GLOBUS_SUCCESS) {
		dprintf(("[%s] Could not start file put\n", name));
		if ( !debug ) syslog(LOG_ERR, "Could not start file put");
		gError = GLOBUS_TRUE;
		gDone = GLOBUS_TRUE;
	} else {
		int rc;
		globus_mutex_lock(&gLock);
		if ( (rc = read(fhnd, gBuffer, BUFSZ)) < 0 ) {
			dprintf(("[%s] Error reading dump file\n", name));
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

	globus_ftp_client_handle_destroy(&hnd);


    return (gError == GLOBUS_TRUE)? 1: 0;
}

/* XXX: we don't use it */
SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };

