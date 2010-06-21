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

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>

#include "soap_version.h"

#include <stdsoap2.h>
#include <glite/security/glite_gsplugin.h>
#include <glite/security/glite_gscompat.h>

#include "jp_.nsmap"
#include "common.h"
#define dprintf(FMT, ARGS...) fprintf(stderr, FMT, ##ARGS);
#include <glite/jp/ws_fault.c>


#define DEFAULT_JPIS "http://localhost:8902"
#define USE_GMT 1


static struct option opts[] = {
	{"index-server",required_argument,	NULL,	'i'},
	{"example-file",required_argument,	NULL,	'e'},
	{"query-file",	required_argument,	NULL,	'q'},
	{"test-file",	required_argument,	NULL,	't'},
	{"format",	required_argument,	NULL,	'f'},
	{NULL, 0, NULL, 0}
};
static const char *get_opt_string = "i:q:e:t:f:";

#define NUMBER_OP 6
struct {
	enum jptype__queryOp op;
	const char *name;
} operations[] = {
	{jptype__queryOp__EQUAL, "=="},
	{jptype__queryOp__UNEQUAL, "<>"},
	{jptype__queryOp__LESS, "<"},
	{jptype__queryOp__GREATER, ">"},
	{jptype__queryOp__WITHIN, "WITHIN"},
	{jptype__queryOp__EXISTS, "EXISTS"},
	{0, "unknown"}
};

#define NUMBER_ORIG 3
struct {
	enum jptype__attrOrig orig;
	const char *name;
} origins[] = {
	{jptype__attrOrig__SYSTEM, "SYSTEM"},
	{jptype__attrOrig__USER, "USER"},
	{jptype__attrOrig__FILE_, "FILE"},
	{0, "unknown"}
};

typedef enum {FORMAT_XML, FORMAT_STRIPPEDXML, FORMAT_HR} format_t;


/*
 * set the value
 */
static void value_set(struct soap *soap, struct jptype__stringOrBlob **value, const char *str) {
	*value = soap_malloc(soap, sizeof(**value));
	memset(*value, 0, sizeof(*value));
	GSOAP_SETSTRING(*value, soap_strdup(soap, str));
}


/*
 * print the query data in the soap structre
 */
static void value_print(FILE *out, const struct jptype__stringOrBlob *value) {
	int i, size, maxsize;
	unsigned char *ptr;

	if (value) {
		if (GSOAP_ISSTRING(value)) fprintf(out, "%s", GSOAP_STRING(value));
		else if (GSOAP_ISBLOB(value)) {
			fprintf(out, "BLOB(");
			ptr = GSOAP_BLOB(value)->__ptr;
			size = GSOAP_BLOB(value)->__size;

			maxsize = 10;
			if (ptr) {
				maxsize = size < 10 ? size : 10;
				for (i = 0; i < maxsize; i++) fprintf(out, "%02X ", ptr[i]);
				if (maxsize < size) fprintf(out, "...");
			} else fprintf(out, "NULL");
			fprintf(out, ")");
		}
	} else {
		fprintf(out, "-");
	}
}


/*
 * fill the query soap structure with some example data
 */
static void query_example_fill(struct soap *soap, struct _jpelem__QueryJobs *in) {
	struct jptype__indexQuery 		*cond;
	struct jptype__indexQueryRecord 	*rec;

	GLITE_SECURITY_GSOAP_LIST_CREATE(soap, in, conditions, struct jptype__indexQuery, 2);
	
	// query status
	cond = GLITE_SECURITY_GSOAP_LIST_GET(in->conditions, 0);
	memset(cond, 0, sizeof(*cond));
	cond->attr = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus");
	cond->origin = soap_malloc(soap, sizeof(*(cond->origin)));
	*(cond->origin) = jptype__attrOrig__SYSTEM;
	GLITE_SECURITY_GSOAP_LIST_CREATE(soap, cond, record, struct jptype__indexQueryRecord, 2);

	// equal to Done
	rec = GLITE_SECURITY_GSOAP_LIST_GET(cond->record, 0);
	memset(rec, 0, sizeof(*rec));
	rec->op = jptype__queryOp__EQUAL;
	value_set(soap, &rec->value, "Done");

	// OR equal to Ready
	rec = GLITE_SECURITY_GSOAP_LIST_GET(cond->record, 1);
	memset(rec, 0, sizeof(*rec));
	rec->op = jptype__queryOp__EQUAL;
	value_set(soap, &rec->value, "Ready");


	// AND
	// owner
	cond = GLITE_SECURITY_GSOAP_LIST_GET(in->conditions, 1);
	memset(cond, 0, sizeof(*cond));
	cond->attr = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:user");
	cond->origin = NULL;
	GLITE_SECURITY_GSOAP_LIST_CREATE(soap, cond, record, struct jptype__indexQueryRecord, 1);

	// not equal to CertSubj
	rec = GLITE_SECURITY_GSOAP_LIST_GET(cond->record, 0);
	memset(rec, 0, sizeof(*rec));
	rec->op = jptype__queryOp__UNEQUAL;
	value_set(soap, &rec->value, "God");


	in->__sizeattributes = 4;
	in->attributes = soap_malloc(soap,
		in->__sizeattributes *
		sizeof(*(in->attributes)));
	in->attributes[0] = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/JP/System:owner");
	in->attributes[1] = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/JP/System:jobId");
	in->attributes[2] = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus");
	in->attributes[3] = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:user");
	
}


/*
 * read the XML query
 */
static int query_recv(struct soap *soap, int fd, struct _jpelem__QueryJobs *qj) {
	int i;

	memset(qj, 0, sizeof(*qj));

	soap->recvfd = fd;
	soap_begin_recv(soap);
	soap_default__jpelem__QueryJobs(soap, qj);
	if (!soap_get__jpelem__QueryJobs(soap, qj, "jpelem:QueryJobs", NULL)) {
		soap_end_recv(soap);
		soap_end(soap);
		return EINVAL;
	}
	soap_end_recv(soap);

	/* strip white-space characters from attributes */
	for (i = 0; i < qj->__sizeattributes; i++)
		glite_jpis_trim(qj->attributes[i]);
	for (i = 0; i < qj->__sizeconditions; i++)
		glite_jpis_trim(GLITE_SECURITY_GSOAP_LIST_GET(qj->conditions, i)->attr);

	return 0;
}


/*
 * print info from the query soap structure
 */
static void query_print(FILE *out, const struct _jpelem__QueryJobs *in) {
	struct jptype__indexQuery	*cond;
	struct jptype__indexQueryRecord 	*rec;
	int i, j, k;

	fprintf(out, "Conditions:\n");
	for (i = 0; i < in->__sizeconditions; i++) {
		cond = GLITE_SECURITY_GSOAP_LIST_GET(in->conditions, i);
		fprintf(out, "\t%s\n", cond->attr);
		if (cond->origin) {
			for (k = 0; k <= NUMBER_ORIG; k++)
				if (origins[k].orig == *(cond->origin)) break;
			fprintf(out, "\t\torigin == %s\n", origins[k].name);
		} else {
			fprintf(out, "\t\torigin IS ANY\n");
		}
		for (j = 0; j < cond->__sizerecord; j++) {
			rec = GLITE_SECURITY_GSOAP_LIST_GET(cond->record, j);
			for (k = 0; k <= NUMBER_OP; k++)
				if (operations[k].op == rec->op) break;
			fprintf(out, "\t\tvalue %s", operations[k].name);
			if (rec->value) {
				fprintf(out, " ");
				value_print(out, rec->value);
			}
			if (rec->value2) {
				if (!rec->value) fprintf(out, "-");
				fprintf(out, " AND ");
				value_print(out, rec->value2);
			}
			fprintf(out, "\n");
		}
	}
	fprintf(out, "Attributes:\n");
	for (i = 0; i < in->__sizeattributes; i++)
		fprintf(out, "\t%s\n", in->attributes[i]);
}


/*
 * dump the XML query
 */
static int query_dump(struct soap *soap, int fd, struct _jpelem__QueryJobs *qj) {
	int retval;

	soap->sendfd = fd;
	soap_begin_send(soap);
	soap_serialize__jpelem__QueryJobs(soap, qj);
	retval = soap_put__jpelem__QueryJobs(soap, qj, "jpelem:QueryJobs", NULL);
	soap_end_send(soap);
	write(fd, "\n", strlen("\n"));

	return retval;
}


static int query_format(struct soap *soap, format_t format, FILE *f, struct _jpelem__QueryJobs *qj) {
	switch (format) {
	case FORMAT_XML:
	case FORMAT_STRIPPEDXML:
		return query_dump(soap, fileno(f), qj);
	case FORMAT_HR:  query_print(f, qj); return 0;
	default: return EINVAL;
	}
}


/*
 * dump the XML query with the example data
 */
static int query_example_dump(struct soap *soap, int fd) {
	struct _jpelem__QueryJobs qj;
	int retval;

	memset(&qj, 0, sizeof(qj));

	soap_begin(soap);
	query_example_fill(soap, &qj);
	retval = query_dump(soap, fd, &qj);
	soap_end(soap);

	return retval;
}


/*
 * dump the data returned from JP IS
 */
static int queryresult_dump(struct soap *soap, int fd, const struct _jpelem__QueryJobsResponse *qjr) {
	int retval;

	soap->sendfd = fd;
	soap_begin_send(soap);
	soap_serialize__jpelem__QueryJobsResponse(soap, qjr);
	retval = soap_put__jpelem__QueryJobsResponse(soap, qjr, "QueryJobsResponse", NULL);
	soap_end_send(soap);
	write(fd, "\n", strlen("\n"));

	return retval;
}


/*
 * print the data returned from JP IS
 */
static void queryresult_print(FILE *out, const struct  _jpelem__QueryJobsResponse *in) {
	struct jptype__jobRecord *job;
	struct jptype__attrValue *attr;
	int i, j, k;

#if USE_GMT
	setenv("TZ","UTC",1); tzset();
#endif
	fprintf(out, "Result %d jobs:\n", in->__sizejobs);
	for (j=0; j<in->__sizejobs; j++) {
		job = GLITE_SECURITY_GSOAP_LIST_GET(in->jobs, j);
		fprintf(out, "\tjobid = %s, owner = %s\n", job->jobid, job->owner);
		for (i=0; i<job->__sizeattributes; i++) {
			attr = GLITE_SECURITY_GSOAP_LIST_GET(job->attributes, i);
			fprintf(out, "\t\t%s\n", attr->name);
			fprintf(out, "\t\t\tvalue = ");
			value_print(out, attr->value);
			fprintf(out, "\n");

			for (k = 0; k <= NUMBER_ORIG; k++)
				if (origins[k].orig == attr->origin) break;
			fprintf(out, "\t\t\torigin = %s", origins[k].name);
			if (attr->originDetail) fprintf(out, ", %s\n", attr->originDetail);
			else fprintf(out, " (no detail)\n");
			if (attr->timestamp != (time_t)0)
				fprintf(out, "\t\t\ttime = %s", ctime(&attr->timestamp));
		}
	}
}


static int queryresult_format(struct soap *soap, format_t format, FILE *f, const struct  _jpelem__QueryJobsResponse *qj) {
	switch (format) {
	case FORMAT_XML:
	case FORMAT_STRIPPEDXML:
		return queryresult_dump(soap, fileno(f), qj);
	case FORMAT_HR:  queryresult_print(f, qj); return 0;
	default: return EINVAL;
	}
}


/*
 * help screen
 */
static void usage(const char *prog_name) {
	fprintf(stderr, "Usage: %s OPTIONS\n", prog_name);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -h|--help\n");
	fprintf(stderr, "  -i|--index-server JPIS:PORT (default: " DEFAULT_JPIS ")\n");
	fprintf(stderr, "  -q|--query-file IN_FILE.XML\n");
	fprintf(stderr, "  -t|--test-file IN_FILE.XML\n");
	fprintf(stderr, "  -e|--example-file OUT_FILE.XML\n");
	fprintf(stderr, "  -f|--format xml | strippedxml | human\n");
}


/*
 * process the result after calling soap
 */
#define check_fault(SOAP, ERR) glite_jp_clientCheckFault((SOAP), (ERR), NULL, 0)


int main(int argc, char * const argv[]) {
	struct soap soap, soap_comm;
	struct _jpelem__QueryJobs qj;
	char *server, *example_file, *query_file, *test_file;
	const char *prog_name;
	int retval, opt, example_fd, query_fd, test_fd;
	format_t format = FORMAT_XML;

	prog_name = server = NULL;
	example_file = query_file = test_file = NULL;
	query_fd = example_fd = test_fd = -1;
	retval = 1;

	soap_init(&soap);
	soap_set_namespaces(&soap, jp__namespaces);

	/* 
	 * Soap with registered plugin can't be used for reading XML.
	 * For communications with JP IS glite_gsplugin needs to be registered yet.
	 */
	soap_init(&soap_comm);
	soap_set_namespaces(&soap_comm, jp__namespaces);
	soap_register_plugin(&soap_comm, glite_gsplugin);

	/* program name */
	prog_name = strrchr(argv[0], '/');
	if (prog_name) prog_name++;
	else prog_name = argv[0];

	if (argc <= 1) {
		usage(prog_name);
		goto cleanup;
	}

	/* handle arguments */
	while ((opt = getopt_long(argc, argv, get_opt_string, opts, NULL)) != EOF) switch (opt) {
		case 'i': 
			free(server);
			server = strdup(optarg);
			break;
		case 'e':
			free(example_file);
			example_file = strdup(optarg);
			break;
		case 'q':
			free(query_file);
			query_file = strdup(optarg);
			break;
		case 't':
			free(test_file);
			test_file = strdup(optarg);
			break;
		case 'f':
			if (strcasecmp(optarg, "xml") == 0) format = FORMAT_XML;
			else if (strcasecmp(optarg, "strippedxml") == 0) format = FORMAT_STRIPPEDXML;
			else format = FORMAT_HR;
			break;
		default:
			usage(prog_name);
			goto cleanup;
	}
	if (optind < argc) {
		usage(prog_name);
		goto cleanup;
	}
	if (!server) server = strdup(DEFAULT_JPIS);
#ifdef SOAP_XML_INDENT
	if (format != FORMAT_STRIPPEDXML) soap_omode(&soap, SOAP_XML_INDENT);
#endif


	/* prepare steps according to the arguments */
	if (query_file) {
		if (strcmp(query_file, "-") == 0) query_fd = STDIN_FILENO;
		else if ((query_fd = open(query_file, 0)) < 0) {
			fprintf(stderr, "error opening %s: %s\n", query_file, strerror(errno));
			goto cleanup;
		}
		free(query_file);
		query_file = NULL;
	}
	if (example_file) {
		if (strcmp(example_file, "-") == 0) example_fd = STDOUT_FILENO;
		else if ((example_fd = creat(example_file, S_IREAD | S_IWRITE | S_IRGRP)) < 0) {
			fprintf(stderr, "error creating %s: %s\n", example_file, strerror(errno));
			goto cleanup;
		}
		free(example_file);
		example_file = NULL;
	}
	if (test_file) {
		if (strcmp(test_file, "-") == 0) test_fd = STDIN_FILENO;
		else if ((test_fd = open(test_file, 0)) < 0) {
			fprintf(stderr, "error opening %s: %s\n", test_file, strerror(errno));
			goto cleanup;
		}
		free(test_file);
		test_file = NULL;
	}

	/* the dump action */
	if (example_fd >= 0) {
		if (query_example_dump(&soap, example_fd) != 0) {
			fprintf(stderr, "Error dumping example query XML.\n");
		}
	}

	/* the test XML file action */
	if (test_fd >= 0) {
		soap_begin(&soap);
		if (query_recv(&soap, test_fd, &qj) != 0) {
			fprintf(stderr, "test: Error getting query XML\n");
		} else {
			query_format(&soap, format, stdout, &qj);
		}
		soap_end(&soap);
	}

	/* query action */
	if (query_fd >= 0) {
		struct _jpelem__QueryJobs in;
		struct _jpelem__QueryJobsResponse out;
		int ret;

		soap_begin(&soap);
		memset(&in, 0, sizeof(in));
		memset(&out, 0, sizeof(out));
		/* 
		 * Right way would be copy data from client query structure to IS query
		 * structure. Just ugly retype to client here.
		 */
		if (query_recv(&soap, query_fd, (struct _jpelem__QueryJobs *)&in) != 0) {
			fprintf(stderr, "query: Error getting query XML\n");
		} else {
			fprintf(stderr, "query: using JPIS %s\n\n", server);
			query_print(stderr, &in);
			fprintf(stderr, "\n");
			soap_begin(&soap_comm);
			ret = check_fault(&soap_comm, soap_call___jpsrv__QueryJobs(&soap_comm, server, "", &in, &out));
			if (ret == 0) {
				queryresult_format(&soap, format, stdout, (struct _jpelem__QueryJobsResponse *)&out);
			} else {
				soap_end(&soap_comm);
				soap_end(&soap);
				goto cleanup;
			}
			soap_end(&soap_comm);
		}
		soap_end(&soap);
	}

	retval = 0;

cleanup:
	soap_done(&soap);
	soap_done(&soap_comm);
	if (example_fd > STDERR_FILENO) close(example_fd);
	if (query_fd > STDERR_FILENO) close(query_fd);
	if (test_fd > STDERR_FILENO) close(test_fd);
	free(server);
	free(example_file);
	free(query_file);
	free(test_file);

	return retval;
}
