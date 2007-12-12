#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"
#include "glite/jp/strmd5.h"
#include "glite/jp/known_attr.h"
#include "glite/jp/attr.h"

#include "feed.h"
#include "tags.h"

#include "utils.h"
#include "backend.h"

/*
 * realloc the line to double size if needed
 *
 * \return 0 if failed, did nothing
 * \return 1 if success
 */
int check_realloc_line(char **line, size_t *maxlen, size_t len) {
        void *tmp;

        if (len > *maxlen) {
                *maxlen <<= 1;
                tmp = realloc(*line, *maxlen);
                if (!tmp) return 0;
                *line = tmp;
        }

        return 1;
}

/*
 * read next line from stream
 *
 * \return error code
 */
int glite_jppsbe_readline(
        glite_jp_context_t ctx,
        void *handle,
        rl_buffer_t *buffer,
        char **line
)
{
        size_t maxlen, len, i;
        ssize_t nbytes;
        int retval, z, end;

        maxlen = BUFSIZ;
        i = 0;
        len = 0;
        *line = malloc(maxlen);
        end = 0;

        do {
                /* read next portion */
                if (buffer->pos >= buffer->size) {
                        buffer->pos = 0;
                        buffer->size = 0;
                        if ((retval = glite_jppsbe_pread(ctx, handle, buffer->buf, BUFSIZ, buffer->offset, &nbytes)) == 0) {
                                if (nbytes < 0) {
                                        retval = EINVAL;
                                        goto fail;
                                } else {
                                        if (nbytes) {
                                                buffer->size = (size_t)nbytes;
                                                buffer->offset += nbytes;
                                        } else end = 1;
                                }
                        } else goto fail;
                }

                /* we have buffer->size - buffer->pos bytes */
                i = buffer->pos;
                do {
                        if (i >= buffer->size) z = '\0';
                        else {
                                z = buffer->buf[i];
                                if (z == '\n') z = '\0';
                        }
                        len++;

                        if (!check_realloc_line(line, &maxlen, len)) {
                                retval = ENOMEM;
                                goto fail;
                        }
                        (*line)[len - 1] = z;
                        i++;
                } while (z && i < buffer->size);
                buffer->pos = i;
        } while (len && (*line)[len - 1] != '\0');

        if ((!len || !(*line)[0]) && end) {
                free(*line);
                *line = NULL;
        }

        return 0;

fail:
        free(*line);
        *line = NULL;
        return retval;
}

char* glite_jpps_get_namespace(const char* attr){
        char* namespace = strdup(attr);
        char* colon = strrchr(namespace, ':');
        if (colon)
                namespace[strrchr(namespace, ':') - namespace] = 0;
        else
                namespace[0] = 0;
        return namespace;
}

