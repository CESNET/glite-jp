#ifndef __GLITE_JP_FEED
#define __GLITE_JP_FEED


struct jpfeed {
	char	*id,*destination;
	time_t	expires;
	glite_jp_attr_t	*attrs;
	glite_jp_query_rec_t	*qry;
	struct jpfeed	*next;
};


int glite_jpps_match_attr(glite_jp_context_t,const char *,const glite_jp_attrval_t[]);
int glite_jpps_match_file(glite_jp_context_t,const char *,glite_jp_fileclass_t);
int glite_jpps_match_tag(glite_jp_context_t,const char *,const glite_jp_tagval_t *);
int glite_jpps_run_feed(glite_jp_context_t,const char *,const glite_jp_attr_t *,const glite_jp_query_rec_t *,char **);
int glite_jpps_register_feed(glite_jp_context_t,const char *,const glite_jp_attr_t *,const glite_jp_query_rec_t *,char **,time_t *);

#endif

