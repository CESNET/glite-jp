#ident "$Header: "

#include <syslog.h>
#include <assert.h>
#include <glite/jp/types.h>
#include <glite/security/glite_gscompat.h>

#ifdef __GNUC__
  #define UNUSED __attribute__((unused))
#else
  #define UNUSED
#endif

#define GSOAP_STRING(CHOICE) GLITE_SECURITY_GSOAP_CHOICE_GET(CHOICE, string, stringOrBlob, 1)
#define GSOAP_BLOB(CHOICE) GLITE_SECURITY_GSOAP_CHOICE_GET(CHOICE, blob, stringOrBlob, 1)
#define GSOAP_SETSTRING(CHOICE, VALUE) GLITE_SECURITY_GSOAP_CHOICE_SET(CHOICE, string, jptype, stringOrBlob, 1, VALUE)
#define GSOAP_SETBLOB(CHOICE, VALUE) GLITE_SECURITY_GSOAP_CHOICE_SET(CHOICE, blob, jptype, stringOrBlob, 1, VALUE)
#define GSOAP_ISSTRING(CHOICE) GLITE_SECURITY_GSOAP_CHOICE_ISTYPE(CHOICE, string, jptype, stringOrBlob, 1)
#define GSOAP_ISBLOB(CHOICE) GLITE_SECURITY_GSOAP_CHOICE_ISTYPE(CHOICE, blob, jptype, stringOrBlob, 1)

#if GSOAP_VERSION >= 20709
  #define GFNUM SOAP_TYPE_jptype__genericFault
#else
  #define GFNUM SOAP_TYPE__genericFault
#endif

#ifndef dprintf
#define dprintf(FMT, ARGS...) printf(FMT, ##ARGS)
#endif


static int glite_jp_clientCheckFault(struct soap *soap, int err, const char *name, int toSyslog) UNUSED;
static struct jptype__genericFault* jp2s_error(struct soap *soap, const glite_jp_error_t *err) UNUSED;
static void glite_jp_server_err2fault(const glite_jp_context_t ctx,struct soap *soap) UNUSED;


static int glite_jp_clientCheckFault(struct soap *soap, int err, const char *name, int toSyslog)
{
	struct SOAP_ENV__Detail *detail;
	struct jptype__genericFault	*f;
	char	*reason,indent[200] = "  ";
	char *prefix;
	int retval;

	if (name) asprintf(&prefix, "[%s] ", name);
	else prefix = strdup("");
	retval = 0;

	switch(err) {
	case SOAP_OK:
		dprintf("%sOK\n", prefix);
		break;

	case SOAP_FAULT:
	case SOAP_SVR_FAULT:
		detail = GLITE_SECURITY_GSOAP_DETAIL(soap);
		reason = GLITE_SECURITY_GSOAP_REASON(soap);
		dprintf("%s%s\n", prefix, reason);
		if (toSyslog) syslog(LOG_ERR, "%s", reason);

		if (detail->__type != GFNUM && detail->__any) {
		// compatibility with clients gSoaps < 2.7.9b
			dprintf("%s%s%s\n", prefix, indent, detail->__any);
			if (toSyslog) syslog(LOG_ERR, "%s", detail->__any);

			f = NULL;
		} else {
		// client is based on gSoap 2.7.9b
			assert(detail->__type == GFNUM);
#if GSOAP_VERSION >= 20709
			f = (struct jptype__genericFault *)detail->fault;
#elif GSOAP_VERSION >= 20700
			f = ((struct _genericFault *)detail->fault)->jpelem__genericFault;
#else
			f = ((struct _genericFault *)detail->value)->jpelem__genericFault;
#endif
		}

		while (f) {
			dprintf("%s%s%s: %s (%s)\n",
					prefix, indent,
					f->source, f->text, f->description);
			if (toSyslog) syslog(LOG_ERR, "%s%s: %s (%s)",
					reason, f->source, f->text, f->description);
			f = f->reason;
			strcat(indent,"  ");
		}
		retval = -1;
		break;

	default:
		soap_print_fault(soap,stderr);
		retval = -1;
	}

	free(prefix);
	return retval;
}


static struct jptype__genericFault* jp2s_error(struct soap *soap, const glite_jp_error_t *err)
{
	struct jptype__genericFault *ret = NULL;
	if (err) {
		ret = soap_malloc(soap,sizeof *ret);
		memset(ret,0,sizeof *ret);
		ret->code = err->code;
		ret->source = soap_strdup(soap,err->source);
		ret->text = soap_strdup(soap,strerror(err->code));
		ret->description = soap_strdup(soap,err->desc);
		ret->reason = jp2s_error(soap,err->reason);
	}
	return ret;
}


static void glite_jp_server_err2fault(const glite_jp_context_t ctx,struct soap *soap)
{
	struct SOAP_ENV__Detail	*detail = soap_malloc(soap,sizeof *detail);
#if GSOAP_VERSION >= 20709
	struct jptype__genericFault *f;
	f = jp2s_error(soap,ctx->error);
#else
	struct _genericFault *f = soap_malloc(soap, sizeof *f);
	f->jpelem__genericFault = jp2s_error(soap,ctx->error);
#endif
	memset(detail, 0, sizeof(*detail));
#if GSOAP_VERSION >= 20700
	detail->fault = (void *)f;
#else
	detail->value = (void *)f;
#endif
	detail->__type = GFNUM;
	detail->__any = NULL;

	soap_receiver_fault(soap,"Oh, shit!",NULL);
	if (soap->version == 2) soap->fault->SOAP_ENV__Detail = detail;
	else soap->fault->detail = detail;
}

#undef UNUSED
