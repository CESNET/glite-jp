#ident "$Header$"
/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners/ for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


#ifndef _CONF_H
#define _CONF_H

#include <glite/jp/types.h>

#ifndef UNUSED
  #ifdef __GNUC__
    #define UNUSED __attribute__((unused))
  #else
    #define UNUSED
  #endif
#endif

#define GLITE_JPIS_DEFAULT_PORT_STR "8902"

//#define lprintf
#define lprintf(args...) glite_jp_lprintf(__FUNCTION__, ##args)
#define llprintf(MODULE, args...) do { \
	if ((MODULE)) glite_jp_lprintf(__FUNCTION__, ##args); \
} while(0)


typedef struct _glite_jp_is_feed {
	char    		*PS_URL;	//URLs of Primary Storage servers
	glite_jp_query_rec_t    *query;		// query to Primary Server (aka filter)
	int     		history, 	// type of query
				continuous;
	long int		uniqueid;       // internal ID
} glite_jp_is_feed;

typedef struct _glite_jp_is_conf {
	// all I need to get from comman line options and configuration file

					// arrays are zero-terminated
	char	**attrs;		// atributes to obtain
	char	**indexed_attrs;	// list of indexed atributes
	char	**multival_attrs;	// list of multivalue attributes
	char	**queriable_attrs;	// list of queriable attributes
	char	**plugins;		// list of plugin.so's

	glite_jp_is_feed	**feeds;	// null terminated list of feeds

	int 	debug;
	int	no_auth;		// set if you do not want authorization
	char	*cs, 			// database contact string
		*port,			// server port
		*pidfile,
		*logfile,
		*server_cert,
		*server_key;
	int	slaves;
	int	delete_db;

	char	*feeding;               // feed DB from local file
	int	force_feed;
} glite_jp_is_conf;



// read commad line options and configuration file
int glite_jp_get_conf(int argc, char **argv, glite_jp_is_conf **configuration);
void glite_jp_free_conf(glite_jp_is_conf *conf);

void glite_jp_lprintf(const char *source, const char *fmt, ...);

#endif
