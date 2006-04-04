#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include <stdsoap2.h>
#include <glite/security/glite_gsplugin.h>

#include "soap_version.h"
#include "jpis_client_.nsmap"


/* 'jpisclient' as future namespace */
#define _jpisclient__QueryJobs _jpelem__QueryJobs
#define _jpisclient__QueryJobsResponse _jpelem__QueryJobsResponse
#define soap_default__jpisclient__QueryJobs soap_default__jpelem__QueryJobs
#define soap_get__jpisclient__QueryJobs soap_get__jpelem__QueryJobs
#define soap_put__jpisclient__QueryJobs soap_put__jpelem__QueryJobs
#define soap_put__jpisclient__QueryJobsResponse soap_put__jpelem__QueryJobsResponse
#define soap_serialize__jpisclient__QueryJobs soap_serialize__jpelem__QueryJobs
#define soap_serialize__jpisclient__QueryJobsResponse soap_serialize__jpelem__QueryJobsResponse

#define DEFAULT_JPIS "http://localhost:8902"


/* namespaces[] not used here but needed to prevent linker to complain... */
SOAP_NMAC struct Namespace namespaces[] = {
	{NULL, NULL, NULL, NULL},
};

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
	{jptype__queryOp__WITHIN, "in"},
	{jptype__queryOp__EXISTS, "exists"},
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

typedef enum {FORMAT_XML, FORMAT_HR} format_t;


/*
 * set the value
 */
static void value_set(struct soap *soap, struct jptype__stringOrBlob **value, const char *str) {
#if GSOAP_VERSION >= 20706
	*value = soap_malloc(soap, sizeof(*value));
	(*value)->__union_1 = SOAP_UNION_jptype__union_1_string;
	(*value)->union_1.string = soap_strdup(soap, str);
#else
	*value = soap_malloc(soap, sizeof(*value));
	(*value)->string = soap_strdup(soap, str);
	(*value)->blob = NULL;
#endif
}


/*
 * print the query data in the soap structre
 */
static void value_print(FILE *out, const struct jptype__stringOrBlob *value) {
	int i, size, maxsize;
	unsigned char *ptr;

	if (value) {
#if GSOAP_VERSION >= 20706
		if (value->__union_1 == SOAP_UNION_jptype__union_1_string) {
			fprintf(out, "%s", value->union_1.string);
		} else if (value->__union_1 == SOAP_UNION_jptype__union_1_blob) {
			fprintf(out, "BLOB(");
			ptr = value->union_1.blob->__ptr;
			size = value->union_1.blob->__size;
#else
		if (value->string) fprintf(out, "%s", value->string);
		else if (value->blob) {
			fprintf(out, "BLOB(");
			ptr = value->blob->__ptr;
			size = value->blob->__size;
#endif
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
static void query_example_fill(struct soap *soap, struct _jpisclient__QueryJobs *in) {
	struct jptype__indexQuery 		*cond;
	struct jptype__indexQueryRecord 	*rec;
	
	in->__sizeconditions = 2;
	in->conditions = soap_malloc(soap,
		in->__sizeconditions * 
		sizeof(*(in->conditions)));
	
	// query status
	cond = soap_malloc(soap, sizeof(*cond));
	memset(cond, 0, sizeof(*cond));
	cond->attr = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:finalStatus");
	cond->origin = NULL;
	cond->__sizerecord = 2;
	cond->record = soap_malloc(soap, cond->__sizerecord * sizeof(*(cond->record)));

	// equal to Done
	rec = soap_malloc(soap, sizeof(*rec));
	memset(rec, 0, sizeof(*rec));
	rec->op = jptype__queryOp__EQUAL;
	value_set(soap, &rec->value, "Done");
	cond->record[0] = rec;

	// OR equal to Ready
	rec = soap_malloc(soap, sizeof(*rec));
	memset(rec, 0, sizeof(*rec));
	rec->op = jptype__queryOp__EQUAL;
	value_set(soap, &rec->value, "Ready");
	cond->record[1] = rec;

	in->conditions[0] = cond;

	// AND
	// owner
	cond = soap_malloc(soap, sizeof(*cond));
	memset(cond, 0, sizeof(*cond));
	cond->attr = soap_strdup(soap, "http://egee.cesnet.cz/en/Schema/LB/Attributes:user");
	cond->origin = NULL;
	cond->__sizerecord = 1;
	cond->record = soap_malloc(soap, cond->__sizerecord * sizeof(*(cond->record)));

	// not equal to CertSubj
	rec = soap_malloc(soap, sizeof(*rec));
	memset(rec, 0, sizeof(*rec));
	rec->op = jptype__queryOp__UNEQUAL;
	value_set(soap, &rec->value, "God");
	cond->record[0] = rec;

	in->conditions[1] = cond;


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
static int query_recv(struct soap *soap, int fd, struct _jpisclient__QueryJobs *qj) {
	memset(qj, 0, sizeof(*qj));

	soap->recvfd = fd;
	soap_begin_recv(soap);
	soap_default__jpisclient__QueryJobs(soap, qj);
	if (!soap_get__jpisclient__QueryJobs(soap, qj, "QueryJobs", NULL)) {
		soap_end_recv(soap);
		soap_end(soap);
		return EINVAL;
	}
	soap_end_recv(soap);
	soap_free(soap); /* don't destroy the data we want */

	return 0;
}


/*
 * print info from the query soap structure
 */
static void query_print(FILE *out, const struct _jpisclient__QueryJobs *in) {
	struct jptype__indexQueryRecord 	*rec;
	int i, j, k;

	fprintf(out, "Conditions:\n");
	for (i = 0; i < in->__sizeconditions; i++) {
		fprintf(out, "\t%s\n", in->conditions[i]->attr);
		for (j = 0; j < in->conditions[i]->__sizerecord; j++) {
			rec = in->conditions[i]->record[j];
			for (k = 0; k <= NUMBER_OP; k++)
				if (operations[k].op == rec->op) break;
			fprintf(out, "\t\t%s", operations[k].name);
			if (rec->value) {
				fprintf(out, " ");
				value_print(out, rec->value);
			}
			if (rec->value2) {
				if (!rec->value) fprintf(out, "-");
				fprintf(out, " ");
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
static int query_dump(struct soap *soap, int fd, struct _jpisclient__QueryJobs *qj) {
	int retval;

	soap->sendfd = fd;
	soap_begin_send(soap);
	soap_serialize__jpisclient__QueryJobs(soap, qj);
	retval = soap_put__jpisclient__QueryJobs(soap, qj, "jpisclient:QueryJobs", NULL);
	soap_end_send(soap);

	return retval;
}


static int query_format(struct soap *soap, format_t format, FILE *f, struct _jpisclient__QueryJobs *qj) {
	switch (format) {
	case FORMAT_XML: return query_dump(soap, fileno(f), qj);
	case FORMAT_HR:  query_print(f, qj); return 0;
	default: return EINVAL;
	}
}


/*
 * dump the XML query with the example data
 */
static int query_example_dump(struct soap *soap, int fd) {
	struct _jpisclient__QueryJobs qj;
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
static int queryresult_dump(struct soap *soap, int fd, const struct _jpisclient__QueryJobsResponse *qjr) {
	int retval;

	soap->sendfd = fd;
	soap_begin_send(soap);
	soap_serialize__jpisclient__QueryJobsResponse(soap, qjr);
	retval = soap_put__jpisclient__QueryJobsResponse(soap, qjr, "jpisclient:QueryJobsResponse", NULL);
	soap_end_send(soap);

	return retval;
}


/*
 * print the data returned from JP IS
 */
static void queryresult_print(FILE *out, const struct  _jpelem__QueryJobsResponse *in) {
	struct jptype__attrValue *attr;
	int i, j, k;

	fprintf(out, "Result %d jobs:\n", in->__sizejobs);
	for (j=0; j<in->__sizejobs; j++) {
		fprintf(out, "\tjobid = %s, owner = %s\n", in->jobs[j]->jobid, in->jobs[j]->owner);
		for (i=0; i<in->jobs[j]->__sizeattributes; i++) {
			attr = in->jobs[j]->attributes[i];
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
	case FORMAT_XML: return queryresult_dump(soap, fileno(f), qj);
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
	fprintf(stderr, "  -f|--format xml | human\n");
}


/*
 * process the result after calling soap
 */
static int check_fault(struct soap *soap, int err) {
	struct SOAP_ENV__Detail *detail;
	struct jptype__genericFault	*f;
	char	*reason,indent[200] = "  ";

	switch(err) {
		case SOAP_OK: fputs("OK", stderr);
			      putc('\n', stderr);
			      break;
		case SOAP_FAULT:
		case SOAP_SVR_FAULT:
			if (soap->version == 2) {
				detail = soap->fault->SOAP_ENV__Detail;
#if GSOAP_VERSION >= 20706
				reason = soap->fault->SOAP_ENV__Reason->SOAP_ENV__Text;
#else
				reason = soap->fault->SOAP_ENV__Reason;
#endif
			}
			else {
				detail = soap->fault->detail;
				reason = soap->fault->faultstring;
			}
			fputs(reason, stderr);
			putc('\n', stderr);
			assert(detail->__type == SOAP_TYPE__genericFault);
#if GSOAP_VERSION >= 20700
			f = ((struct _genericFault *) detail->fault)
#else
			f = ((struct _genericFault *) detail->value)
#endif
				-> jpelem__genericFault;

			while (f) {
				fprintf(stderr,"%s%s: %s (%s)\n",indent,
						f->source,f->text,f->description);
				f = f->reason;
				strcat(indent,"  ");
			}
			return -1;

		default: soap_print_fault(soap,stderr);
			 return -1;
	}
	return 0;
}


int main(int argc, char * const argv[]) {
	struct soap soap, soap_comm;
	struct _jpisclient__QueryJobs qj;
	char *server, *example_file, *query_file, *test_file;
	const char *prog_name;
	int retval, opt, example_fd, query_fd, test_fd;
	struct Namespace *namespaces;
	int i;
	format_t format = FORMAT_XML;

	prog_name = server = NULL;
	example_file = query_file = test_file = NULL;
	query_fd = example_fd = test_fd = -1;
	retval = 1;

	soap_init(&soap);

#ifdef SOAP_XML_INDENT
	soap_omode(&soap, SOAP_XML_INDENT);
#endif

	/*
	 * Following code is needed, when we can't combine more XSD/WSDL files
	 * for using both as client. We direct use structures only from
	 * JobProvenanceIS.wsdl, just retyped to jpisclient namespace.
	 *
	 * So manually add jpisclient to the namespaces.
	 */
	for (i = 0; jpis_client__namespaces[i].id; i++)
		if (strcmp(jpis_client__namespaces[i].id, "jpisclient") == 0) break;
	if (jpis_client__namespaces[i].id) {
		/* 
		 * namespaces hack isn't needed (used two schemas and
		 * gsoap 2.7.6b)
		 */
		namespaces = NULL;
		soap_set_namespaces(&soap, jpis_client__namespaces);
	} else {
		/* perform namespaces hack */
		namespaces = calloc(i + 2, sizeof(struct Namespace));
		memcpy(namespaces, jpis_client__namespaces, sizeof(jpis_client__namespaces));
		namespaces[i].id = "jpisclient";
		namespaces[i].ns = "http://glite.org/xsd/types/jpisclient";
		soap_set_namespaces(&soap, namespaces);
	}

	/* 
	 * Soap with registered plugin can't be used for reading XML.
	 * For communications with JP IS glite_gsplugin needs to be registered yet.
	 */
	soap_init(&soap_comm);
	soap_set_namespaces(&soap_comm, jpis_client__namespaces);
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
		if (query_recv(&soap, query_fd, (struct _jpisclient__QueryJobs *)&in) != 0) {
			fprintf(stderr, "query: Error getting query XML\n");
		} else {
			fprintf(stderr, "query: using JPIS %s\n\n", server);
			query_print(stderr, &in);
			fprintf(stderr, "\n");
			soap_begin(&soap_comm);
			ret = check_fault(&soap_comm, soap_call___jpsrv__QueryJobs(&soap_comm, server, "", &in, &out));
			if (ret == 0) {
				queryresult_format(&soap, format, stdout, (struct _jpisclient__QueryJobsResponse *)&out);
			} else goto cleanup;
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
	free(namespaces);

	return retval;
}
