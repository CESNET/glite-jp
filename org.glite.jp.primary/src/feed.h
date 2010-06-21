/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners/ for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef GLITE_JP_FEED_H
#define GLITE_JP_FEED_H


struct jpfeed {
/* feed data */
	char	*id,*destination;
	time_t	expires;
	int	continuous;

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
int glite_jpps_match_attr_multi(glite_jp_context_t,const char **, const glite_jp_attrval_t **);
int glite_jpps_match_tag(glite_jp_context_t,const char *,const char *,const char *);
int glite_jpps_run_feed(glite_jp_context_t,const char *,char const * const *,const glite_jp_query_rec_t *,int,char **);
int glite_jpps_register_feed(glite_jp_context_t,const char *,char const * const *,const glite_jp_query_rec_t *,char **,time_t *);

#endif /* GLITE_JP_FEED_H */
