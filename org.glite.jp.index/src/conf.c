/* Module for obtaining configuration for Index Server */

#ident "$Header$"

#include <stdlib.h>
#include <string.h>

#include "conf.h"


int glite_jp_get_conf(int argc, char **argv, char *config_file, glite_jp_is_conf **configuration)
{ 
        // read comman line options and configuration file
	// XXX: use EGEE global configure tools in future...

	glite_jp_is_conf	*conf;


	conf = calloc(1, sizeof(*conf));

	conf->attrs = calloc(5, sizeof(*conf->attrs));
	conf->attrs[0] = strdup("owner");
	conf->attrs[1] = strdup("status");
	conf->attrs[2] = strdup("location");
	conf->attrs[3] = strdup("jobid");

	conf->indexed_attrs = calloc(3, sizeof(*conf->indexed_attrs));
	conf->indexed_attrs[0] = strdup("owner");
	conf->indexed_attrs[1] = strdup("location");

	// XXX: some plugin names should come here in future
	conf->plugins = NULL;


	/* ask for one feed */
	conf->feeds = calloc(2, sizeof(*(conf->feeds)));
	
	conf->feeds[0] = calloc(1, sizeof(**(conf->feeds)));
	conf->feeds[0]->PS_URL = strdup("http://localhost:8901");

	// all job since Epoche
	conf->feeds[0]->query = calloc(2,sizeof(*conf->feeds[0]->query));
	conf->feeds[0]->query[0] = calloc(2,sizeof(**conf->feeds[0]->query));
	conf->feeds[0]->query[0][0].attr = strdup("date");
	conf->feeds[0]->query[0][0].op = GLITE_JP_QUERYOP_GREATER;
	conf->feeds[0]->query[0][0].value = strdup("0");

	conf->feeds[0]->history = 0;
	conf->feeds[0]->continuous = 1;

	conf->feeds[1] = NULL;


	*configuration = conf;

        return 0; 
} 


void glite_jp_free_conf(glite_jp_is_conf *conf)
{
	// XXX: structure dealocation
}
