#ifndef __GLITE_JP_FEED
#define __GLITE_JP_FEED


struct jpfeed {
/* feed data */
	char	*id,*destination;
	time_t	expires;

/* complete and split query and attribute list */
	char	**attrs,**meta_attr,**other_attr;
	int	int_other_attr; /* index from where other_attr is extended
				  with attributes from other_query */ 

	int	nother_attr, nmeta_attr, nmeta_qry, nother_qry;
	glite_jp_query_rec_t	*qry,*meta_qry,*other_qry;

/* jobs stacked for feed */
	int	njobs;
	char	**jobs;
	char	**owners;
	glite_jp_attrval_t	**job_attrs;

/* next feed */
	struct jpfeed	*next;
};


int glite_jpps_match_attr(glite_jp_context_t,const char *,const glite_jp_attrval_t[]);
int glite_jpps_match_file(glite_jp_context_t,const char *,const char *,const char *);
int glite_jpps_match_tag(glite_jp_context_t,const char *,const char *,const char *);
int glite_jpps_run_feed(glite_jp_context_t,const char *,char const * const *,const glite_jp_query_rec_t *,char **);
int glite_jpps_register_feed(glite_jp_context_t,const char *,char const * const *,const glite_jp_query_rec_t *,char **,time_t *);

#endif

