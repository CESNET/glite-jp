/* Module for obtaining configuration for Index Server */

#ident "$Header$"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <glite/jp/types.h>
#include <glite/jp/context.h>
#include "conf.h"


int glite_jp_get_conf(int argc, char **argv, char *config_file, glite_jp_is_conf **configuration)
{
	char *debug;
	char *ps = NULL;

        // read comman line options and configuration file
	// XXX: use EGEE global configure tools in future...

	glite_jp_is_conf	*conf;


	conf = calloc(1, sizeof(*conf));

	// configuration from environment	
	conf->cs = getenv("GLITE_JPIS_DB");
	conf->port = getenv("GLITE_JPIS_PORT");
	debug = getenv("GLITE_JPIS_DEBUG");
	conf->debug = (debug != NULL) && (strcmp(debug, "0") != 0);
	conf->no_auth = 1;				// check authorization
	conf->pidfile = getenv("GLITE_JPIS_PIDFILE");
	conf->logfile = getenv("GLITE_JPIS_LOGFILE");

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

//	ps = "http://umbar.ics.muni.cz:8901";
	if (!ps && ((ps = getenv("GLITE_JPIS_PS")) == NULL)) {
		printf("No JP PrimaryStrorage server specified in $GLITE_JPIS_PS, default feeds skipped. (not fatal)\n");
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

	conf->feeds[0]->history = 1;
	conf->feeds[0]->continuous = 0;

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
