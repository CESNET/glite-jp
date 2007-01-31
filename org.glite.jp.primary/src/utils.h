#ifndef __GLITE_JP_UTILS
#define __GLITE_JP_UTILS

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

#endif

