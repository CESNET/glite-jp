#ident "$Header: "

#include <syslog.h>
#include <glite/jp/types.h>
#include <glite/security/glite_gscompat.h>

#if GSOAP_VERSION >= 20709
  #define GSOAP_FAULT(DETAIL) ((DETAIL)->jpelem__genericFault)
#elif GSOAP_VERSION >= 20700
  #define GSOAP_FAULT(DETAIL) (((struct _genericFault *)(DETAIL)->fault)->jpelem__genericFault)
#else
  #define GSOAP_FAULT(DETAIL) (((struct _genericFault *)(DETAIL)->value)->jptype__genericFault)
#endif
#define GSOAP_STRING(CHOICE) GLITE_SECURITY_GSOAP_CHOICE_GET(CHOICE, string, jptype__stringOrBlob, 1)
#define GSOAP_BLOB(CHOICE) GLITE_SECURITY_GSOAP_CHOICE_GET(CHOICE, blob, jptype__stringOrBlob, 1)

#ifndef dprintf
#define dprintf(x) printf x
#endif

static int glite_jp_clientCheckFault(struct soap *soap, int err, const char *name, int toSyslog) {
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
		dprintf(("%sOK\n", prefix));
		break;

	case SOAP_FAULT:
	case SOAP_SVR_FAULT:
		detail = GLITE_SECURITY_GSOAP_DETAIL(soap);
		reason = GLITE_SECURITY_GSOAP_REASON(soap);
		dprintf(("%s%s\n", prefix, reason));
		if (toSyslog) syslog(LOG_ERR, "%s", reason);
		if (!(
#ifdef SOAP_TYPE__genericFault
		(detail->__type == SOAP_TYPE__genericFault) ||
#endif
#ifdef SOAP_TYPE_jptype__genericFault
		(detail->__type == SOAP_TYPE_jptype__genericFault) ||
#endif
#ifdef SOAP_TYPE_lbt__genericFault
		(detail->__type == SOAP_TYPE_lbt__genericFault) ||
#endif
		0)) assert(1);
		f = GSOAP_FAULT(detail);

		while (f) {
			dprintf(("%s%s%s: %s (%s)\n",
					prefix, indent,
					f->source, f->text, f->description));
			if (toSyslog) syslog(LOG_ERR, "%s%s: %s (%s)",
					reason, f->source, f->text, f->description);
			f = f->reason;
			strcat(indent,"  ");
		}
		retval = -1;

	default:
		soap_print_fault(soap,stderr);
		retval = -1;
	}

	free(prefix);
	return retval;
}


static struct jptype__genericFault *jp2s_error(struct soap *soap, const glite_jp_error_t *err)
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
	struct _genericFault *f = soap_malloc(soap,sizeof *f);

	f->jpelem__genericFault = jp2s_error(soap,ctx->error);

	detail->__type = SOAP_TYPE__genericFault;
	GSOAP_FAULT(detail) = f;
	detail->__any = NULL;

	soap_receiver_fault(soap,"Oh, shit!",NULL);
	if (soap->version == 2) soap->fault->SOAP_ENV__Detail = detail;
	else soap->fault->detail = detail;
}
