#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

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

int glite_jpps_tagval_copy(
	glite_jp_context_t ctx,
	glite_jp_tagval_t *from,
	glite_jp_tagval_t *to
)
{
	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	to->name = strdup(from->name);
	if (!to->name) {
		err.code = ENOMEM;
		return glite_jp_stack_error(ctx,&err);
	}
	to->sequence = from->sequence;
	to->timestamp = from->timestamp;
	to->binary = from->binary;
	to->size = from->size;
	to->value = (char *) malloc(to->size);
	if (!to->value) {
		free(to->name);
		err.code = ENOMEM;
		return glite_jp_stack_error(ctx,&err);
	}
	memcpy(from->value, to->value, to->size);

	return 0;
}

int glite_jpps_tag_read(
	glite_jp_context_t ctx,
	void *handle,
	off_t offset,
	glite_jp_tagval_t *tagvalue,
	size_t *shift
)
{
	char hdr[HEADER_SIZE+1];
	unsigned int nlen;
	unsigned long  vlen;
	char binary;
	unsigned sequence;
	unsigned timestamp;
	char * name = NULL;
	char * value = NULL;
	ssize_t ret;
	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	hdr[HEADER_SIZE] = '\0';
	if (glite_jppsbe_pread(ctx, handle, hdr, HEADER_SIZE, offset, &ret)) {
		err.code = EIO;
		err.desc = "Cannot read tag header";
		goto error_out;
	}
	if (ret == 0) {
		err.code = ENOENT;
		err.desc = "No more tags in the file";
		goto error_out;
	}
	/* #define HEADER "JP#TAG# %05u %012lu %c %05u %012lu#" */
	if (sscanf(hdr, HEADER, &nlen, &vlen, &binary, &sequence, &timestamp) < 5) {
		err.code = EILSEQ;
		err.desc = "Incorrect tag header format";
		goto error_out;
	}
	name = (char*) malloc(nlen + 1);
	if (!name) {
		err.code = ENOMEM;
		goto error_out;
	}
	name[nlen] = '\0';
	value = (char*) malloc(vlen + 1);
	if (!value) {
		err.code = ENOMEM;
		goto error_out;
	}
	value[vlen] = '\0';
	if (glite_jppsbe_pread(ctx, handle, name, nlen, offset + HEADER_SIZE, &ret)) {
		err.code = EIO;
		err.desc = "Cannot read tag name";
		goto error_out;
	}
	if (glite_jppsbe_pread(ctx, handle, value, vlen, offset + HEADER_SIZE + nlen, &ret)) {
		err.code = EIO;
		err.desc = "Cannot read tag value";
		goto error_out;
	}
	
	tagvalue->name = name;
	tagvalue->sequence = sequence;
	tagvalue->timestamp = timestamp;
	tagvalue->binary = (binary == 'B') ? 1 : 0;
	tagvalue->size = vlen;
	tagvalue->value = value;

	*shift = HEADER_SIZE + nlen + vlen;

	return 0;
error_out:
	free(name);
	free(value);
	return glite_jp_stack_error(ctx,&err);
}

/*
int glite_jpps_tag_read(glite_jp_context_t, void *, off_t, glite_jp_tagval_t *, size_t);
int glite_jpps_tag_readall(glite_jp_context_t, void *, glite_jp_tagval_t **);
*/

int glite_jpps_tag_readall(
	glite_jp_context_t ctx,
	void *handle,
	glite_jp_tagval_t **tags_out
)
{
	glite_jp_tagval_t * tags = NULL;
	void * newspace;
	int ntags = 0;
	int ntagspace = 0;
	off_t offset = 0;
	int ret;
	size_t shift;
	glite_jp_error_t err;

	glite_jp_clear_error(ctx);
	memset(&err,0,sizeof err);
	err.source = __FUNCTION__;

	ntagspace = 1;
	tags = (glite_jp_tagval_t *) calloc(ntagspace + 1, sizeof(*tags));
	if (!tags) {
		err.code = ENOMEM;
		return glite_jp_stack_error(ctx,&err);
	}
	while (!(ret = glite_jpps_tag_read(ctx, handle, offset, &tags[ntags], &shift))) {
		offset += shift;
		ntags++;
		if (ntagspace <= ntags) {
			ntagspace += 1;
			newspace = realloc(tags, (ntagspace + 1) * sizeof(*tags));
			if (!newspace) {
				err.code = ENOMEM;
				goto error_out;
			}
			tags = (glite_jp_tagval_t *) newspace;
		}
	}
	if (ret == ENOENT) {
		*tags_out = tags;
		return 0;
	} else {
		err.code = EIO;
		err.desc = "Error reading tag value";
	}

error_out:
	for (; ntags-- ;) {
		free(tags[ntags].name);
		free(tags[ntags].value);
	}
	free(tags);
	return glite_jp_stack_error(ctx,&err);
}
