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
#include "ws_is_typeref.h"


#define SOAP_FMAC3 static
#define WITH_NOGLOBAL
#include "jpis_C.c"

extern SOAP_NMAC struct Namespace jpis__namespaces[];

static const char *get_opt_string = "dq:c:k:C:V:nm:p:i:o:x:";

static struct option opts[] = {
	{"debug",       0, NULL,	'd'},
	{"query-type",	1, NULL,	'q'},
	{"cert",	1, NULL,	'c'},
	{"key",		1, NULL,	'k'},
//	{"CAdir",       1, NULL,	'C'},
//	{"VOMSdir",     1, NULL,	'V'},
	{"noauth",      0, NULL,	'n'},
	{"mysql",       1, NULL,	'm'},
	{"port",	1, NULL,	'p'},
	{"pidfile",     1, NULL,	'i'},
	{"logfile",     1, NULL,	'o'},
	{"config",      1, NULL,	'x'},
	{NULL,		0, NULL,	0}
};

static int read_conf(glite_jp_is_conf *conf, char *conf_file);
static int dump_conf(void);

static void usage(char *me) 
{
	fprintf(stderr,"usage: %s [option]\n"
		"\t-d, --debug\t don't run as daemon, additional diagnostics\n"
		"\t-q, --query-type hist/cont/both (default history)\n"
		"\t-k, --key\t private key file\n" 
		"\t-c, --cert\t certificate file\n"
//		"\t-C, --CAdir\t trusted certificates directory\n"
//		"\t-V, --VOMSdir\t trusted VOMS servers certificates directory\n"
		"\t-n, --noauth\t don't check user identity with result owner\n"
		"\t-m, --mysql\t database connect string\n"
		"\t-p, --port\t port to listen\n"
		"\t-i, --pidfile\t file to store master pid\n"
		"\t-o, --logfile\t file to store logs\n"
		"\t-x, --config\t file with server configuration\n"
		"\n"
	,me);
}


int glite_jp_get_conf(int argc, char **argv, char *config_file, glite_jp_is_conf **configuration)
{
	char 			*ps = NULL, *qt = NULL, *conf_file = NULL;
	int			opt;
	glite_jp_is_conf	*conf;


	conf = calloc(1, sizeof(*conf));


	while ((opt = getopt_long(argc,argv,get_opt_string,opts,NULL)) != EOF) switch (opt) {
		case 'd': conf->debug = 1; break;
		case 'q': qt = optarg; break;
		case 'c': conf->server_cert = optarg; break;
		case 'k': conf->server_key = optarg; break;
//		case 'C': cadir = optarg; break;
//		case 'V': vomsdir = optarg; break;
		case 'n': conf->no_auth = 1; break;
		case 'm': conf->cs = optarg; break;
		case 'p': conf->port = optarg; break;
		case 'i': conf->pidfile = optarg; break;
		case 'o': conf->logfile = optarg; break;
		case 'x': conf_file = optarg; break;
		default : usage(argv[0]); exit(0); break;
	}

	if (!conf->cs) {
			fprintf(stderr,"DB contact string not specified! "\
			 "Using build-in default:  %s \n", GLITE_JP_IS_DEFAULTCS);
	}
	if (!conf->port) {
			fprintf(stderr,"JP IS port not specified! "\
			"Using build-in default:  %s \n", GLITE_JPIS_DEFAULT_PORT_STR);
	}

	if (!conf_file) {
		fprintf(stderr,"JP IS configuration file must be specified! "\
			"Exiting.\n");
		return 1;
	}
	else {
		read_conf(conf, conf_file);
	}

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

/*
 * Reads configuration from XML conf. file
 */
static int read_conf(glite_jp_is_conf *conf, char *conf_file)
{
	struct soap					soap;
	struct 	_jpelem__ServerConfigurationResponse 	out;
	int						fd, i;


	if ((fd = open(conf_file, 0)) < 0) {
       		fprintf(stderr, "error opening %s: %s\n", conf_file, strerror(errno));
                return 1;
        }

	soap_init(&soap);
	soap_set_namespaces(&soap, jpis__namespaces);

	soap_begin(&soap);
	soap.recvfd = fd;
	soap_begin_recv(&soap);
	memset(&out, 0, sizeof(out));
					
	if (!soap_get__jpelem__ServerConfigurationResponse(&soap, &out, "ServerConfiguration", NULL)) {
                soap_end_recv(&soap);
                soap_end(&soap);
                return EINVAL;
        }
	soap_end_recv(&soap);

	if (out.__sizeattrs) {
		conf->attrs = calloc(out.__sizeattrs + 1, sizeof(*conf->attrs));
		for (i=0; i < out.__sizeattrs; i++) {
			conf->attrs[i] = strdup(out.attrs[i]);
		}
	}
	if (out.__sizeindexedAttrs) {
		conf->indexed_attrs = calloc(out.__sizeindexedAttrs + 1, sizeof(*conf->indexed_attrs));
		for (i=0; i < out.__sizeindexedAttrs; i++) {
			conf->indexed_attrs[i] = strdup(out.indexedAttrs[i]);
		}
	}
	if (out.__sizeplugins) {
		conf->plugins = calloc(out.__sizeplugins + 1, sizeof(*conf->plugins));
		for (i=0; i < out.__sizeplugins; i++) {
			conf->plugins[i] = strdup(out.plugins[i]);
		}
	}
	if (out.__sizefeeds) {
#warning FIXME: list
		conf->feeds = calloc(out.__sizefeeds + 1, sizeof(*conf->feeds));
		for (i=0; i < out.__sizefeeds; i++) {
			conf->feeds[i] = calloc(1, sizeof(*conf->feeds[i]));
			conf->feeds[i]->PS_URL=strdup(out.feeds[i]->primaryServer);

			if (out.feeds[i]->__sizecondition) {
				glite_jpis_SoapToPrimaryQueryConds(&soap, out.feeds[i]->__sizecondition,
					out.feeds[i]->condition, &conf->feeds[i]->query);
			}
			
			conf->feeds[i]->history = out.feeds[0]->history;
			conf->feeds[i]->continuous = out.feeds[0]->continuous;
		}
	}

	soap_destroy(&soap);
	soap_end(&soap);
	soap_done(&soap);

	return 0;
}

/*
 * Just helper function - used only once for first generation
 * of XML example configuration (which was then reedited in hand)
 */
static int dump_conf(void) {
        int retval;
	struct  _jpelem__ServerConfigurationResponse    out;
	struct soap                                     soap;
	

	soap_init(&soap);
        soap_set_namespaces(&soap, jpis__namespaces);

        soap.sendfd = STDOUT_FILENO;
        soap_begin_send(&soap);
	soap_default__jpelem__ServerConfigurationResponse(&soap, &out);

	out.__sizeattrs = 2;
	out.attrs = calloc(2, sizeof(*out.attrs));
	out.attrs[0] = strdup("atrr1"); 
	out.attrs[1] = strdup("atrr2"); 

	out.__sizeindexedAttrs = 2;
	out.indexedAttrs = calloc(2, sizeof(*out.indexedAttrs));
	out.indexedAttrs[0] = strdup("idxAtrr1"); 
	out.indexedAttrs[1] = strdup("idxAtrr2"); 

	out.__sizeplugins = 2;
        out.plugins = calloc(2, sizeof(*out.plugins));
        out.plugins[0] = strdup("plugin1");
        out.plugins[1] = strdup("plugin2");

#warning: FIXME: lists
	out.__sizefeeds = 1;
        out.feeds = calloc(1, sizeof(*out.feeds));
	out.feeds[0] = calloc(1, sizeof(*out.feeds[0]));
        out.feeds[0]->primaryServer = strdup("PrimaryServer");
	out.feeds[0]->__sizecondition = 1;
	out.feeds[0]->condition = calloc(1, sizeof(*(out.feeds[0]->condition)) );
	out.feeds[0]->condition[0] = calloc(1, sizeof(*(out.feeds[0]->condition[0])) );
	out.feeds[0]->condition[0]->attr = strdup("queryAttr");
	out.feeds[0]->condition[0]->op = jptype__queryOp__EQUAL;
	out.feeds[0]->condition[0]->origin = jptype__attrOrig__SYSTEM;
	out.feeds[0]->condition[0]->value = calloc(1, sizeof(*(out.feeds[0]->condition[0]->value)) );
	out.feeds[0]->condition[0]->value->string = strdup("attrValue");
	out.feeds[0]->history = 1;
	out.feeds[0]->continuous = 0; 

        soap_serialize__jpelem__ServerConfigurationResponse(&soap, &out);
        retval = soap_put__jpelem__ServerConfigurationResponse(&soap, &out, "jpelem:ServerConfiguration", NULL);
        soap_end_send(&soap);
	soap_free(&soap);
	soap_end(&soap);

        return retval;
}

