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

#ident "$Header: "

#include <syslog.h>
#include <assert.h>
#include <glite/jp/types.h>
#include <glite/security/glite_gscompat.h>

#ifndef UNUSED
  #ifdef __GNUC__
    #define UNUSED __attribute__((unused))
  #else
    #define UNUSED
  #endif
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
static int glite_jp_clientGetErrno(struct soap *soap, int err) UNUSED;
static void glite_jp_server_err2fault(const glite_jp_context_t ctx,struct soap *soap) UNUSED;

static struct jptype__genericFault* jp2s_error(struct soap *soap, const glite_jp_error_t *err);
static int clientGetFault(struct soap *soap, int err, const char **reason, struct jptype__genericFault **f, const char **fallback);


/*
 * get client fault structs
 *   err - code got from soap call
 *   reason - error text
 *   f - extended fault structs or NULL
 *   fallback - xml fault description or NULL
 * return values:
 *   0 - OK
 *   1 - got a extended fault info
 *   2 - internal gsoap fault
 */
static int clientGetFault(struct soap *soap, int err, const char **reason, struct jptype__genericFault **f, const char **fallback) {
	struct SOAP_ENV__Detail *detail;

	*f = NULL;
	if (fallback) *fallback = NULL;

	switch(err) {
	case SOAP_OK:
		return 0;

	case SOAP_FAULT:
	case SOAP_SVR_FAULT:
		detail = GLITE_SECURITY_GSOAP_DETAIL(soap);
		if (reason) *reason = GLITE_SECURITY_GSOAP_REASON(soap);

		if (!detail) return 1;
		if (detail->__type != GFNUM && detail->__any) {
		// compatibility with clients gSoaps < 2.7.9b
			if (fallback) *fallback = detail->__any;
			return 1;
		}
		// client is based on gSoap 2.7.9b
		assert(detail->__type == GFNUM);
#if GSOAP_VERSION >= 20709
		*f = (struct jptype__genericFault *)detail->fault;
#elif GSOAP_VERSION >= 20700
		*f = ((struct _genericFault *)detail->fault)->jpelem__genericFault;
#else
		*f = ((struct _genericFault *)detail->value)->jpelem__genericFault;
#endif
		return 1;

	default:
		return 2;
	}
}


static int glite_jp_clientGetErrno(struct soap *soap, int err) {
	struct jptype__genericFault	*f;

	switch(clientGetFault(soap, err, NULL, &f, NULL)) {
	case 0: return 0;
	case 1: return f ? f->code : -2;
	default: return -1;
	}
}


static int glite_jp_clientCheckFault(struct soap *soap, int err, const char *name, int toSyslog)
{
	struct jptype__genericFault	*f;
	const char	*reason, *xml;
	char	indent[200] = "  ";
	char *prefix;
	int retval;

	if (name) asprintf(&prefix, "[%s] ", name);
	else prefix = strdup("");

	switch(clientGetFault(soap, err, &reason, &f, &xml)) {
	case 0:
		retval = 0;
		dprintf("%sOK\n", prefix);
		break;

	case 1:
		retval = -1;
		dprintf("%s%s\n", prefix, reason);
		if (toSyslog) syslog(LOG_ERR, "%s", reason);
		if (!f && xml) {
			dprintf("%s%s%s\n", prefix, indent, xml);
			if (toSyslog) syslog(LOG_ERR, "%s", xml);
		}
		while (f) {
			dprintf("%s%s%s: %s (%s)\n",
					prefix, indent,
					f->source, f->text, f->description);
			if (toSyslog) syslog(LOG_ERR, "%s%s: %s (%s)",
					prefix, f->source, f->text, f->description);
			f = f->reason;
			strcat(indent,"  ");
		}
		break;

	case 2:
		fprintf(stderr, "%ssoap err=%d, ", prefix, err);
		soap_print_fault(soap, stderr);
		retval = -1;
		break;
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
		ret->description = err->desc ? soap_strdup(soap,err->desc) : NULL;
		ret->reason = jp2s_error(soap,err->reason);
	}
	return ret;
}


static void glite_jp_server_err2fault(const glite_jp_context_t ctx,struct soap *soap)
{
	struct SOAP_ENV__Detail	*detail;
	struct jptype__genericFault *item;
#if GSOAP_VERSION >= 20709
	struct jptype__genericFault *f;
	item = f = jp2s_error(soap,ctx->error);
#else
	struct _genericFault *f = soap_malloc(soap, sizeof *f);
	item = f->jpelem__genericFault = jp2s_error(soap,ctx->error);
#endif
	soap_receiver_fault(soap,"Oh, shit!",NULL);
	// no error in JP context?
	if (!item) return;

	detail = (struct SOAP_ENV__Detail *)soap_faultdetail(soap);
#if GSOAP_VERSION >= 20700
	detail->fault = (void *)f;
#else
	detail->value = (void *)f;
#endif
	detail->__type = GFNUM;
	detail->__any = NULL;

	if (soap->version == 2) soap->fault->SOAP_ENV__Detail = detail;
	else soap->fault->detail = detail;
}
