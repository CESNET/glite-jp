#ifndef __GLITE_JP_CONTEXT
#define __GLITE_JP_CONTEXT

int glite_jp_init_context(glite_jp_context_t *);
void glite_jp_free_query_rec(glite_jp_query_rec_t *);

char *glite_jp_peer_name(glite_jp_context_t);
char *glite_jp_error_chain(glite_jp_context_t);

int glite_jp_stack_error(glite_jp_context_t, const glite_jp_error_t *);
int glite_jp_clear_error(glite_jp_context_t); 

int glite_jp_add_deferred(glite_jp_context_t,int (*)(glite_jp_context_t,void *),void *);
int glite_jp_run_deferred(glite_jp_context_t);


#endif
