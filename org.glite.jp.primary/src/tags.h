struct tags_handle {
        void    *bhandle;
        int     n;
        glite_jp_attrval_t      *tags;
};

int tag_append(void *fpctx,void *bhandle,glite_jp_attrval_t * tag);
//int glite_jpps_tag_append(glite_jp_context_t,void *,const char *, const char *);
//int glite_jpps_tag_append(glite_jp_context_t,void *,const glite_jp_tagval_t *);
int tag_attr(void *fpctx,void *handle,const char *attr,glite_jp_attrval_t **attrval);
