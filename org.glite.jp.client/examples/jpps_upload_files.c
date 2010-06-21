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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "jp_client.h"
#include "jpimporter.h"

static char *myname;

void usage(void)
{
	fprintf(stderr,
			"Usage: %s [-h][-p user_proxy][-j jobid] file [file ...]\n"
			"    -h                 show this help\n"
			"    -p <file>          path to the proxy filename\n"
			"    -j <jobid>         jobid string\n"
			"    -m <dir>           location of the lb maildir structure\n"
			"    -s <addr:port>     JP PS server address and port\n"
			, myname);
}

int main(int argc, char **argv)
{
	glite_jpcl_context_t	ctx;
	char				  **files,
						   *jobid = NULL,
						   *proxy = NULL,
						   *lbmd = NULL,
						   *jpps = NULL;
	int						i, j;


	myname = strrchr(argv[0],'/');
	if ( myname ) myname++; else myname = argv[0];

	if ( argc < 2 ) { usage(); return 1; }
	for ( i = 1; i < argc; i++ ) {
		if ( argv[i][0] != '-' ) break;
		if ( argv[i][1] == 'j' ) jobid = argv[++i];
		else if ( argv[i][1] == 'p' ) proxy = argv[++i];
		else if ( argv[i][1] == 'm' ) lbmd = argv[++i];
		else if ( argv[i][1] == 's' ) jpps = argv[++i];
		else if ( argv[i][1] == 'h' || argv[i][1] == '?' ) {usage();return 0;}
		else {usage();return 1;}
	}

	if ( i >= argc ) { usage(); return 1; }

	if ( !proxy && !(proxy = getenv("X509_USER_PROXY")) ) {
		perror("-p or X509_USER_PROXY must be set!\n");
		return 1;
	}

	if ( !(files = calloc(argc-i+1, sizeof(*files))) ) {
		perror("calloc()");
		return 1;
	}
	j = 0;
	while ( i < argc ) files[j++] = argv[i++];

	if ( glite_jpcl_InitContext(&ctx) ) {
		perror("glite_jpcl_InitContext()");
		return 1;
	}

	if ( lbmd ) glite_jpcl_SetParam(ctx, GLITE_JPCL_PARAM_LBMAILDIR, lbmd);
	if ( jpps ) glite_jpcl_SetParam(ctx, GLITE_JPCL_PARAM_JPPS, jpps);

	if ( glite_jpimporter_upload_files(ctx, jobid, (const char **)files, proxy) ) {
		char *errt, *errd;

		glite_jpcl_Error(ctx, &errt, &errd);
		printf("Error calling glite_jpimporter_upload_files()\n\t%s: %s\n",
				errt, errd);
		glite_jpcl_FreeContext(ctx);
		return 1;
	}

	glite_jpcl_FreeContext(ctx);
	return 0;
}
