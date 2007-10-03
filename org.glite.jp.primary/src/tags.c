#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <stdint.h>

#include <glite/lbu/trio.h>
#include <glite/jp/types.h>
#include "tags.h"
#include "backend.h"

/* magic name_len value_len binary sequence timestamp */
#define HEADER "JP#TAG# %05u %012lu %c %05u %012lu#"
#define HEADER_SIZE 48
#define TAGS_MAGIC 0x74c016f2   /* two middle digits encode version, i.e. 01 */

static int tagsread(void *fpctx,struct tags_handle *h);

/*int glite_jpps_tag_append(
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
	// #define HEADER "JP#TAG# %05u %012lu %c %05u %012lu#" 
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
}*/

/*
int glite_jpps_tag_read(glite_jp_context_t, void *, off_t, glite_jp_tagval_t *, size_t);
int glite_jpps_tag_readall(glite_jp_context_t, void *, glite_jp_tagval_t **);
*/

/*int glite_jpps_tag_readall(
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
}*/

int tag_append(void *fpctx,void *bhandle,glite_jp_attrval_t * tag)
{
        //va_list ap;
        char    *hdr,*rec;
        glite_jp_context_t      ctx = fpctx;
        uint32_t                magic,hlen,rlen,rlen_n;
        size_t                  r;
        glite_jp_error_t        err;

        memset(&err,0,sizeof err);
        err.source = __FUNCTION__;
        glite_jp_clear_error(ctx);

        printf("tagappend: %s,%s\n",tag->name,tag->value);

        //assert(oper == GLITE_JP_FPLUG_TAGS_APPEND);

        if (glite_jppsbe_pread(ctx,bhandle,&magic,sizeof magic,0,&r)) {
                err.code = EIO;
                err.desc = "reading magic number";
                return glite_jp_stack_error(ctx,&err);
        }

        if (r == 0) {
                magic = htonl(TAGS_MAGIC);
                if (glite_jppsbe_pwrite(ctx,bhandle,&magic,sizeof magic,0)) {
                        err.code = EIO;
                        err.desc = "writing magic number";
                        return glite_jp_stack_error(ctx,&err);
                }
        }
        else if (r != sizeof magic) {
                err.code = EIO;
                err.desc = "can't read magic number";
                return glite_jp_stack_error(ctx,&err);
        }
        else if (magic != htonl(TAGS_MAGIC)) {
                err.code = EINVAL;
                err.desc = "invalid magic number";
                return glite_jp_stack_error(ctx,&err);
        }

/* XXX: origin is always USER, not recorded */
        trio_asprintf(&hdr,"%ld %c",
                        tag->timestamp,tag->binary ? 'B' : 'S');

        rlen = strlen(tag->name) + strlen(hdr) + 2 /* \0 after name and after hdr */ +
                (r = tag->binary ? tag->size : (tag->value ? strlen(tag->value) : 0));

        rlen_n = htonl(rlen);

        rec = malloc(rlen + sizeof rlen_n);
        *((uint32_t *) rec) = rlen_n;
        strcpy(rec + sizeof rlen_n,tag->name);
        strcpy(rec + (hlen = sizeof rlen_n + strlen(tag->name) + 1),hdr);

        if (r) memcpy(rec + hlen + strlen(hdr) + 1,tag->value,r);
        free(hdr);

/* record format:
 * - 4B length, net byte order
 * - attr name, \0
 * - %ld %c \0 (timestamp, B/S)
 * - value
 */
        if (glite_jppsbe_append(ctx,bhandle,rec,rlen + sizeof rlen_n)) {
                err.code = EIO;
                err.desc = "writing tag record";
                free(rec);
                return glite_jp_stack_error(ctx,&err);
        }

        /* XXX: should add tag also to handle->tags, but it is never used
         * currently */

        return 0;
}

int tag_attr(void *fpctx,void *handle,const char *attr,glite_jp_attrval_t **attrval)
{
        struct tags_handle      *h = handle;
        glite_jp_error_t        err;
        glite_jp_context_t      ctx = fpctx;
        glite_jp_attrval_t      *out = NULL;
        int     i,nout = 0;

        memset(&err,0,sizeof err);
        err.source = __FUNCTION__;

        if (!h->tags) tagsread(fpctx,handle);

        if (!h->tags) {
                err.code = ENOENT;
                err.desc = "no tags for this job";
                return glite_jp_stack_error(ctx,&err);
        }

        for (i=0; i<h->n; i++) if (!strcmp(h->tags[i].name,attr)) {
                out = realloc(out,(nout+2) * sizeof *out);
                glite_jp_attrval_copy(out+nout,h->tags+i);
                nout++;
                memset(out+nout,0,sizeof *out);
        }

        if (nout) {
                *attrval = out;
                return 0;
        }
        else {
                err.code = ENOENT;
                err.desc = "no value for this tag";
                return glite_jp_stack_error(ctx,&err);
        }
}

static int tagsread(void *fpctx,struct tags_handle *h)
{
        glite_jp_context_t      ctx = fpctx;
        uint32_t                magic,rlen;
        glite_jp_error_t        err;
        int                     r;
        size_t                  off = sizeof rlen;
        glite_jp_attrval_t      *tp;
        char                    *rp;

        memset(&err,0,sizeof err);
        err.source = __FUNCTION__;

        glite_jp_clear_error(ctx);

// read magic number
        if (glite_jppsbe_pread(ctx,h->bhandle,&magic,sizeof magic,0,&r)) {
                err.code = EIO;
                err.desc = "reading magic number";
                return glite_jp_stack_error(ctx,&err);
        }

        if (r != sizeof magic) {
                err.code = EIO;
                err.desc = "can't read magic number";
                return glite_jp_stack_error(ctx,&err);
        }
        else if (magic != htonl(TAGS_MAGIC)) {
                err.code = EINVAL;
                err.desc = "invalid magic number";
                return glite_jp_stack_error(ctx,&err);
        }


        while (1) {
                char    *rec,type;
                int     rd;

	// read record header
                if (glite_jppsbe_pread(ctx,h->bhandle,&rlen,sizeof rlen,off,&r)) {
                        err.code = EIO;
                        err.desc = "reading record header";
                        return glite_jp_stack_error(ctx,&err);
                }
                if (r == 0) break;

                if (r != sizeof rlen) {
                        err.code = EIO;
                        err.desc = "can't read record header";
                        return glite_jp_stack_error(ctx,&err);
                }

                off += r;
                rec = malloc(rlen = ntohl(rlen));

        // read whole record body thoroughly
                for (rd=0; rd<rlen; rd+=r) // XXX: will loop on 0 bytes read
                        if (glite_jppsbe_pread(ctx,h->bhandle,rec+rd,rlen-rd,off+rd,&r)) {
                                err.code = EIO;
                                err.desc = "reading record body";
                                free(rec);
                                return glite_jp_stack_error(ctx,&err);
                        }

                off += rlen;

        // parse the record
                h->tags = realloc(h->tags,(h->n+2) * sizeof *h->tags);
                tp = h->tags+h->n++;
                memset(tp,0,sizeof *tp);

                tp->name = strdup(rec);
                rp = rec + strlen(rec) + 1;

                sscanf(rp,"%ld %c",&tp->timestamp,&type);
                rp += strlen(rp) + 1;
                switch (type) {
                        int     i;

                        case 'B': tp->binary = 1; break;
                        case 'S': tp->binary = 0; break;
                        default: free(rec);
                                 for (i=0; i<h->n; i++)
                                         glite_jp_attrval_free(h->tags+i,0);
                                 free(h->tags);
                                 h->tags = NULL;
                                 h->n = 0;

                                 err.code = EINVAL;
                                 err.desc = "invalid attr type (B/S)";
                                 return glite_jp_stack_error(ctx,&err);
                }
                tp->value = malloc((r=rlen - (rp - rec)) + 1);
                memcpy(tp->value,rp,r);
                if (!tp->binary) tp->value[r] = 0;
                tp->origin = GLITE_JP_ATTR_ORIG_USER;

                free(rec);
        }
        return 0;
}

