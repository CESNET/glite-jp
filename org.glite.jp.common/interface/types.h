#ifndef __GLITE_JP_TYPES
#define __GLITE_JP_TYPES

#include <sys/time.h>

typedef struct _glite_jp_error_t {
	int	code;
	char	*desc;
	char	*source;
	struct _glite_jp_error_t *reason;
} glite_jp_error_t;

typedef struct _glite_jp_context {
	glite_jp_error_t *error;
	int	(**deferred_func)(struct _glite_jp_context *,void *);
	void	**deferred_arg;
	void	*feeds;
	struct soap	*other_soap;
} *glite_jp_context_t;

typedef enum {
	GLITE_JP_FILECLASS_UNDEF,
	GLITE_JP_FILECLASS_INPUT,
	GLITE_JP_FILECLASS_OUTPUT,
	GLITE_JP_FILECLASS_LBLOG,
	GLITE_JP_FILECLASS_TAGS,
	GLITE_JP_FILECLASS__LAST
} glite_jp_fileclass_t;

typedef struct {
	char	*name;
	int	sequence;
	time_t	timestamp;
	int	binary;
	size_t	size;
	char	*value;
} glite_jp_tagval_t;

typedef enum {
	GLITE_JP_ATTR_UNDEF,
	GLITE_JP_ATTR_OWNER,
	GLITE_JP_ATTR_TIME,
	GLITE_JP_ATTR_TAG,
	GLITE_JP_ATTR__LAST
} glite_jp_attrtype_t;

typedef struct {
	glite_jp_attrtype_t	type;
	char	*name;
} glite_jp_attr_t;

typedef struct {
	glite_jp_attr_t	attr;
	union {
		char	*s;
		int	i;
		struct timeval time;
		glite_jp_tagval_t tag;
	} value;
} glite_jp_attrval_t;


typedef enum {
	GLITE_JP_QUERYOP_UNDEF,
	GLITE_JP_QUERYOP_EQUAL,
	GLITE_JP_QUERYOP_UNEQUAL,
	GLITE_JP_QUERYOP_LESS,
	GLITE_JP_QUERYOP_GREATER,
	GLITE_JP_QUERYOP_WITHIN,
	GLITE_JP_QUERYOP__LAST,
} glite_jp_queryop_t;

typedef struct {
	glite_jp_attr_t attr;
	glite_jp_queryop_t op;
	union _glite_jp_query_rec_val {
		char	*s;
		int	i;
		struct timeval time;
	} value,value2;
} glite_jp_query_rec_t;

#endif
