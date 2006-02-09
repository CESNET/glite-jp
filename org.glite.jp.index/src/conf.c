/* Module for obtaining configuration for Index Server */

#ident "$Header$"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <getopt.h>
#include <unistd.h>


#include <glite/jp/types.h>
#include <glite/jp/context.h>
#include "conf.h"
#include "db_ops.h"


static const char *get_opt_string = "s:dq:c:k:C:V:nm:p:i:o:";

static struct option opts[] = {
	{"is-server",	1, NULL,	's'},
	{"debug",       0, NULL,	'd'},
	{"query-type",	1, NULL,	'q'},
//	{"cert",	1, NULL,	'c'},
//	{"key",		1, NULL,	'k'},
//	{"CAdir",       1, NULL,	'C'},
//	{"VOMSdir",     1, NULL,	'V'},
	{"noauth",      0, NULL,	'n'},
	{"mysql",       1, NULL,	'm'},
	{"port",	1, NULL,	'p'},
	{"pidfile",     1, NULL,	'i'},
	{"logfile",     1, NULL,	'o'},
	{NULL,		0, NULL,	0}
};



static void usage(char *me) 
{
	fprintf(stderr,"usage: %s [option]\n"
		"\t-s, --ps-server\t primary storage server address (http://hostname:port)\n"
		"\t-d, --debug\t don't run as daemon, additional diagnostics\n"
		"\t-q, --query-type hist/cont/both (default history)\n"
//		"\t-k, --key\t private key file\n" 
//		"\t-c, --cert\t certificate file\n"
//		"\t-C, --CAdir\t trusted certificates directory\n"
//		"\t-V, --VOMSdir\t trusted VOMS servers certificates directory\n"
		"\t-n, --noauth\t don't check user identity with result owner\n"
		"\t-m, --mysql\t database connect string\n"
		"\t-p, --port\t port to listen\n"
		"\t-i, --pidfile\t file to store master pid\n"
		"\t-o, --logfile\t file to store logs\n"
		"\n"
	,me);
}


int glite_jp_get_conf(int argc, char **argv, char *config_file, glite_jp_is_conf **configuration)
{
	char 			*env, *ps = NULL, *qt = NULL;;
	int			opt;
	glite_jp_is_conf	*conf;


	conf = calloc(1, sizeof(*conf));


	while ((opt = getopt_long(argc,argv,get_opt_string,opts,NULL)) != EOF) switch (opt) {
		case 's': ps = optarg; break;
		case 'd': conf->debug = 1; break;
		case 'q': qt = optarg; break;
//		case 'c': server_cert = optarg; break;
//		case 'k': server_key = optarg; break;
//		case 'C': cadir = optarg; break;
//		case 'V': vomsdir = optarg; break;
		case 'n': conf->no_auth = 1; break;
		case 'm': conf->cs = optarg; break;
		case 'p': conf->port = optarg; break;
		case 'i': conf->pidfile = optarg; break;
		case 'o': conf->logfile = optarg; break;
		default : usage(argv[0]); exit(0); break;
	}

	// *** legacy configuration from environment ***********************
	//
	if (!ps) {
		if (env = getenv("GLITE_JPIS_PS")) {
			ps = env;
		}
		else { 
			fprintf(stderr,"No JP PrimaryStrorage server specified, default feeds skipped. (not fatal)\n");
		}
	}

	if (!conf->debug) {
        	env = getenv("GLITE_JPIS_DEBUG");
        	conf->debug = (env != NULL) && (strcmp(env, "0") != 0);
	}

	if (!conf->cs) {
		if (env = getenv("GLITE_JPIS_DB")) {
			conf->cs = env;
		}
		else { 
			fprintf(stderr,"DB contact string not specified! "\
			 "Using build-in default:  %s \n", GLITE_JP_IS_DEFAULTCS);
		}
	}
	if (!conf->port) {
		if (env = getenv("GLITE_JPIS_PORT")) {
			conf->port = env;
		}
		else {
			fprintf(stderr,"JP IS port not specified! "\
			"Using build-in default:  %s \n", GLITE_JPIS_DEFAULT_PORT_STR);
		}
	}
	if (!conf->pidfile) 
		conf->pidfile = getenv("GLITE_JPIS_PIDFILE");
	if (!conf->logfile) 
		conf->logfile = getenv("GLITE_JPIS_LOGFILE");
	//
	// *****************************************************************


	// prefixes & attributes defined in:
	// lb.server/build/jp_job_attrs.h (created when build plugin)
	// jp.common/interfaces/known_attr.h

	conf->attrs = calloc(19, sizeof(*conf->attrs));
	conf->attrs[0] = strdup("http://egee.cesnet.cz/en/Schema/JP/System:owner");
	conf->attrs[1] = strdup("http://egee.cesnet.cz/en/Schema/JP/System:jobId");
	conf->attrs[2] = strdup("http://egee.cesnet.cz/en/Schema/JP/System:regtime");
	conf->attrs[3] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:user");
	conf->attrs[4] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:aTag");
	conf->attrs[5] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:eNodes");
	conf->attrs[6] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:RB");
	conf->attrs[7] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:CE");
	conf->attrs[8] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:UIHost");
	conf->attrs[9] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:CPUTime");
	conf->attrs[10] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:NProc");
	conf->attrs[11] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus");
	conf->attrs[12] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatusDate");
	conf->attrs[13] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:retryCount");
	conf->attrs[14] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:jobType");
	conf->attrs[15] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:nsubjobs");
	conf->attrs[16] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:lastStatusHistory");
	conf->attrs[17] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:fullStatusHistory");

	conf->indexed_attrs = calloc(8, sizeof(*conf->indexed_attrs));
	conf->indexed_attrs[0] = strdup("http://egee.cesnet.cz/en/Schema/JP/System:owner");
	conf->indexed_attrs[1] = strdup("http://egee.cesnet.cz/en/Schema/JP/System:jobId");
	conf->indexed_attrs[2] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:user");
	conf->indexed_attrs[3] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus");
	conf->indexed_attrs[4] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:UIHost");
	conf->indexed_attrs[5] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:CE");
	conf->indexed_attrs[6] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:RB");

	// XXX: some plugin names should come here in future
	conf->plugins = NULL;

	if (!ps) {
		// No JP PrimaryStrorage server specified in $GLITE_JPIS_PS -> skip feeds
		conf->feeds = calloc(1, sizeof(*(conf->feeds)));
		*configuration = conf;
		return 0;
	}

	/* ask for one feed */
	conf->feeds = calloc(2, sizeof(*(conf->feeds)));
	
	conf->feeds[0] = calloc(1, sizeof(**(conf->feeds)));
	conf->feeds[0]->PS_URL = strdup(ps);

	// all job since Epoche
	conf->feeds[0]->query = calloc(2,sizeof(*conf->feeds[0]->query));
	conf->feeds[0]->query[0] = calloc(2,sizeof(**conf->feeds[0]->query));
	conf->feeds[0]->query[0][0].attr = strdup("http://egee.cesnet.cz/en/Schema/JP/System:regtime");
	conf->feeds[0]->query[0][0].op = GLITE_JP_QUERYOP_GREATER;
	conf->feeds[0]->query[0][0].value = strdup("0");

	if (qt && !strcmp(qt,"both")) {
		conf->feeds[0]->history = 1;
		conf->feeds[0]->continuous = 1;
	}
	else if (qt && !strcmp(qt,"continuous")) {
		conf->feeds[0]->history = 0;
		conf->feeds[0]->continuous = 1;
	}
	else {
		conf->feeds[0]->history = 1;
		conf->feeds[0]->continuous = 0;
	}

	conf->feeds[1] = NULL;

	*configuration = conf;

        return 0; 
} 


void glite_jp_free_conf(glite_jp_is_conf *conf)
{
	size_t i, j, k;
	glite_jp_is_feed *feed;

	if (!conf) return;

	if (conf->attrs) for (i = 0; conf->attrs[i]; i++) free(conf->attrs[i]);
	if (conf->indexed_attrs) for (i = 0; conf->indexed_attrs[i]; i++) free(conf->indexed_attrs[i]);
	if (conf->plugins) for (i = 0; conf->plugins[i]; i++) free(conf->plugins[i]);
	if (conf->feeds) for (i = 0; conf->feeds[i]; i++) {
		feed = conf->feeds[i];
		free(feed->PS_URL);
		for (j = 0; feed->query[j]; j++) {
			for (k = 0; feed->query[j][k].attr; k++) glite_jp_free_query_rec(&feed->query[j][k]);
			free(feed->query[j]);
		}
		free(feed->query);
		free(feed);
	}
	free(conf->attrs);
	free(conf->indexed_attrs);
	free(conf->plugins);
	free(conf->feeds);
	free(conf);
}


void glite_jp_lprintf(const char *source, const char *fmt, ...) {
	va_list ap;

	printf("%s: ", source);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}
