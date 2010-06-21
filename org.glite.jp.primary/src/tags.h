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

struct tags_handle {
        void    *bhandle;
        int     n;
        glite_jp_attrval_t      *tags;
};

int tag_append(void *fpctx,void *bhandle,glite_jp_attrval_t * tag);
//int glite_jpps_tag_append(glite_jp_context_t,void *,const char *, const char *);
//int glite_jpps_tag_append(glite_jp_context_t,void *,const glite_jp_tagval_t *);
int tag_attr(void *fpctx,void *handle,const char *attr,glite_jp_attrval_t **attrval);
