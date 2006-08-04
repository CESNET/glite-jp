#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/* TODO: belongs to server-bones */

int glite_jpis_daemonize(const char *servername, const char *custom_pidfile, const char *custom_logfile) {
	int lfd, opid;
	FILE *fpid;
	pid_t master;
	char *pidfile, *logfile;

	if (!custom_logfile) {
		asprintf(&logfile, "%s/%s.log", geteuid() == 0 ? "/var/log" : getenv("HOME"), servername);
	} else {
		logfile = NULL;
	}
	lfd = open(logfile ? logfile : custom_logfile,O_CREAT|O_TRUNC|O_WRONLY,0600);
	if (lfd < 0) {
		fprintf(stderr,"%s: %s: %s\n",servername,logfile,strerror(errno));
		free(logfile);
		return 0;
	}
//	printf("logfile: %s\n", logfile ? logfile : custom_logfile);
	free(logfile);

	if (daemon(0,1) == -1) {
		perror("can't daemonize");
		return 0;
	}
	dup2(lfd,1);
	dup2(lfd,2);

	if (!custom_pidfile) {
		asprintf(&pidfile, "%s/%s.pid", geteuid() == 0 ? "/var/run" : getenv("HOME"), servername);
	} else {
		pidfile = strdup(custom_pidfile);
	}
//	printf("pidfile: %s\n", pidfile ? pidfile : custom_pidfile);
	setpgrp(); /* needs for signalling */
	master = getpid();
	fpid = fopen(pidfile,"r");
	if ( fpid )
	{
		opid = -1;

		if ( fscanf(fpid,"%d",&opid) == 1 )
		{
			if ( !kill(opid,0) )
			{
				fprintf(stderr,"%s: another instance running, pid = %d\n",servername,opid);
				return 0;
			}
			else if (errno != ESRCH) { perror("kill()"); return 0; }
		}
		fclose(fpid);
	} else if (errno != ENOENT) { perror(pidfile); free(pidfile); return 0; }

	if (((fpid = fopen(pidfile, "w")) == NULL) || 
	    (fprintf(fpid, "%d", getpid()) <= 0) ||
	    (fclose(fpid) != 0)) { 
		perror(pidfile); 
		free(pidfile); 
		return 0;
	}

	free(pidfile);
	return 1;
}
