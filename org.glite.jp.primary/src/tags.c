#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <glite/jp/types.h>
#include "tags.h"
#include "backend.h"

/* magic name_len value_len binary sequence timestamp */
#define HEADER "JP#TAG# %05u %012lu %c %05u %012lu#"
#define HEADER_SIZE 48

int glite_jpps_tag_append(
	glite_jp_context_t ctx,
	void *handle,
	const glite_jp_tagval_t *tag
)
{
	char	hdr[HEADER_SIZE+1];
	glite_jp_error_t	err;

	unsigned long	vlen = tag->binary ? tag->size :
				(tag->value ? strlen(tag->value) : 0);
	int	nlen;

	memset(&err,0,sizeof err);
	err.source = "glite_jpps_tag_append()";

	if (!tag->name) {
		err.code = EINVAL;
		err.desc = "tag name";
		return glite_jp_stack_error(ctx,&err);
	}

	nlen = strlen(tag->name);

	assert(sprintf(hdr,HEADER,nlen,vlen,
			tag->binary ? "B" : "S",
			tag->sequence, tag->timestamp) == HEADER_SIZE);

	if (glite_jppsbe_append(ctx,handle,hdr,HEADER_SIZE)) {
		err.code = EIO;
		err.desc = "write tag header";
		return glite_jp_stack_error(ctx,&err);
	}

	if (glite_jppsbe_append(ctx,handle,tag->name,nlen)) {
		err.code = EIO;
		err.desc = "write tag name";
		return glite_jp_stack_error(ctx,&err);
	}

	if (glite_jppsbe_append(ctx,handle,tag->value,vlen)) {
		err.code = EIO;
		err.desc = "write tag value";
		return glite_jp_stack_error(ctx,&err);
	}

	return 0;
}

