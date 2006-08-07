#ident "$Header$"

#include <string.h>
#include <stdsoap2.h>

#include "common.h"

#define WHITE_SPACE_SET "\n\r \t"


void glite_jpis_trim_soap(struct soap *soap, char **soap_str) {
	size_t pos, len;
	char *s;

	if (!*soap_str) return;

	pos = strspn(*soap_str, WHITE_SPACE_SET);
	len = strcspn(*soap_str + pos, WHITE_SPACE_SET);
	s = soap_malloc(soap, len + 1);
	memcpy(s, *soap_str + pos, len);
	s[len] = '\0';

	soap_dealloc(soap, *soap_str);
	*soap_str = s;
}
