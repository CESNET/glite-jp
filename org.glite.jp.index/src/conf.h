#ifndef _CONF_H
#define _CONF_H

#ident "$Header$"


typedef struct _glite_jp_is_conf {
	// all I need to get from comman line options and configuration file
} glite_jp_is_conf;



// read commad line options and configuration file
int glite_jp_get_conf(int argc, char **argv, char *config_file, glite_jp_is_conf *conf);


#endif
