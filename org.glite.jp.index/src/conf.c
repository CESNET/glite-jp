/* Module for obtaining configuration for Index Server */

#ident "$Header$"

#include <stdlib.h>
#include <string.h>

#include <glite/jp/types.h>
#include <glite/jp/context.h>
#include "conf.h"


int glite_jp_get_conf(int argc, char **argv, char *config_file, glite_jp_is_conf **configuration)
{ 
        // read comman line options and configuration file
	// XXX: use EGEE global configure tools in future...

	glite_jp_is_conf	*conf;


	conf = calloc(1, sizeof(*conf));
	
	// prefixes & attributes defined in:
	// lb.server/build/jp_job_attrs.h (created when build plugin)
	// jp.common/interfaces/known_attr.h

	conf->attrs = calloc(4, sizeof(*conf->attrs));
	conf->attrs[0] = strdup("http://egee.cesnet.cz/en/Schema/JP/System:owner");
	conf->attrs[1] = strdup("http://egee.cesnet.cz/en/Schema/JP/System:jobId");
	conf->attrs[2] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus");

	conf->indexed_attrs = calloc(3, sizeof(*conf->indexed_attrs));
	conf->indexed_attrs[0] = strdup("http://egee.cesnet.cz/en/Schema/JP/System:owner");
	conf->indexed_attrs[1] = strdup("http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus");

	// XXX: some plugin names should come here in future
	conf->plugins = NULL;


	/* ask for one feed */
	conf->feeds = calloc(2, sizeof(*(conf->feeds)));
	
	conf->feeds[0] = calloc(1, sizeof(**(conf->feeds)));
	conf->feeds[0]->PS_URL = strdup("http://umbar.ics.muni.cz:8901");
//	conf->feeds[0]->PS_URL = strdup("http://localhost:8901");

	// all job since Epoche
	conf->feeds[0]->query = calloc(2,sizeof(*conf->feeds[0]->query));
	conf->feeds[0]->query[0] = calloc(2,sizeof(**conf->feeds[0]->query));
	conf->feeds[0]->query[0][0].attr = strdup("date");
	conf->feeds[0]->query[0][0].op = GLITE_JP_QUERYOP_GREATER;
	conf->feeds[0]->query[0][0].value = strdup("0");

	conf->feeds[0]->history = 0;
	conf->feeds[0]->continuous = 1;

	conf->feeds[1] = NULL;

	conf->cs = getenv("GLITE_JPIS_DB");
	conf->port = getenv("GLITE_JPIS_PORT");

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
