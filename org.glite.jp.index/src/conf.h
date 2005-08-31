#ident "$Header$"

#ifndef _CONF_H
#define _CONF_H


#include "glite/jp/types.h"


typedef struct _glite_jp_is_conf {
	// all I need to get from comman line options and configuration file

					// arrays are zero-terminated
	char	**attrs;		// atributes to obtain
	char 	**PS_list;		// URLs of Primary Storage servers
	char	**indexed_attrs;	// list of indexed atributes

	glite_jp_query_rec_t	**query;	// query to Primary Server

	int	history, continuous;	// type of query
} glite_jp_is_conf;



// read commad line options and configuration file
int glite_jp_get_conf(int argc, char **argv, char *config_file, glite_jp_is_conf **configuration);
void glite_jp_free_conf(glite_jp_is_conf *conf);

#endif