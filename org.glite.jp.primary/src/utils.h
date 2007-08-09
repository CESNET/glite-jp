#ifndef GLITE_JP_UTILS_H
#define GLITE_JP_UTILS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "feed.h"

typedef struct _rl_buffer_t {
        char                    *buf;
        size_t                  pos, size;
        off_t                   offset;
} rl_buffer_t;

int glite_jppsbe_readline(
        glite_jp_context_t ctx,
        void *handle,
        rl_buffer_t *buffer,
        char **line
);

char* glite_jpps_get_namespace(
	const char* attr
);

#endif /* GLITE_JP_UTILS_H */
