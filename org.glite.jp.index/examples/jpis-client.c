#include <getopt.h>
#include <stdsoap2.h>

/* workaround namespace symbol naming */
#define jpis_client__namespaces namespaces
#include "jpis_client_.nsmap"


static struct option opts[] = {
	{"host",	1,	NULL,	'h'},
	{NULL, 0, NULL, 0}
};
static const char *get_opt_string = "h:";


#if USE_DUMP
void query_example_fill(struct soap *soap, struct _jpisclient__QueryJobs *in) {
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
	rec->value = soap_malloc(soap, sizeof(*(rec->value)));
	rec->value->string = soap_strdup(soap, "Done");
	rec->value->blob = NULL;
	cond->record[0] = rec;

	// OR equal to Ready
	rec = soap_malloc(soap, sizeof(*rec));
	memset(rec, 0, sizeof(*rec));
	rec->op = jptype__queryOp__EQUAL;
	rec->value = soap_malloc(soap, sizeof(*(rec->value)));
	rec->value->string = soap_strdup(soap, "Ready");
	rec->value->blob = NULL;
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
	rec->value = soap_malloc(soap, sizeof(*(rec->value)));
	rec->value->string = soap_strdup(soap, "God");
	rec->value->blob = NULL;
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


void query_example_free(struct soap *soap, struct _jpisclient__QueryJobs *in) {
	struct jptype__indexQueryRecord 	*rec;
	int i, j;

	for (i = 0; i < in->__sizeconditions; i++) {
		if (in->conditions[i]->attr) soap_dealloc(soap, in->conditions[i]->attr);
		for (j = 0; j < in->conditions[i]->__sizerecord; j++) {
			rec = in->conditions[i]->record[j];
			if (rec->value) {
				if (rec->value->string) soap_dealloc(soap, rec->value->string);
				if (rec->value->blob) soap_dealloc(soap, rec->value->blob);
				soap_dealloc(soap, rec->value);
			}
			soap_dealloc(soap, in->conditions[i]->record[j]);
		}
		soap_dealloc(soap, in->conditions[i]);
	}
	for (i = 0; i < in->__sizeattributes; i++)
		soap_dealloc(soap, in->attributes[i]);
}


void query_example_dump(struct soap *soap, int fd) {
	struct _jpisclient__QueryJobs qj;

	memset(&qj, 0, sizeof(qj));

	soap->sendfd = fd;
	soap_begin(soap);
	query_example_fill(soap, &qj);
	soap_begin_send(soap);
	soap_serialize__jpisclient__QueryJobs(soap, &qj);
	soap_put__jpisclient__QueryJobs(soap, &qj, "queryJobs", "indexQuery");
	soap_end_send(soap);
	soap_end(soap);
}
#endif

int query_recv(struct soap *soap, int fd, struct _jpisclient__QueryJobs *qj) {
	memset(qj, 0, sizeof(*qj));

	soap->recvfd = fd;
	soap_begin_recv(soap);
	soap_default__jpisclient__QueryJobs(soap, qj);
	if (!soap_get__jpisclient__QueryJobs(soap, qj, "queryJobs", "indexQuery")) {
		soap_end_recv(soap);
		soap_end(soap);
		return EINVAL;
	}
	soap_end_recv(soap);
	soap_free(soap); /* don't destroy the data we want */

	return 0;
}


static void usage(const char *name) {
	fprintf(stderr, "Usage: %s [-h JPIS:PORT]\n", name);
}


int main(int argc, char * const argv[]) {
	struct soap soap;
	struct _jpisclient__QueryJobs qj;
	char *query_file = NULL;
	const char *name = NULL;
	int opt;

	soap_init(&soap);
#ifdef SOAP_XML_INDENT
	soap_omode(&soap, SOAP_XML_INDENT);
#endif

#ifdef USE_DUMP
	query_example_dump(&soap, 1);
#endif

	/* arguments */
	name = strrchr(argv[0], '/');
	if (name) name++;
	else name = argv[0];

	while ((opt = getopt_long(argc, argv, get_opt_string, opts, NULL)) != EOF) switch (opt) {
		case 'h': free(query_file); query_file = strdup(optarg); break;
		default:
			usage(name);
			return 1;
	}
	if (optind < argc) {
		usage(name);
		return 1;
	}

	soap_begin(&soap);
	if (query_recv(&soap, 0, &qj) != 0) {
		printf("chyba\n");
	} else {
		printf("qj.attributes[0] = %s\n", qj.attributes[0]);
	}
	soap_end(&soap); /* free decoded data */

	soap_done(&soap);

	return 0;
}
