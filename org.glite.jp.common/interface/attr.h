#ifndef __GLITE_JP_ATTR
#define __GLITE_JP_ATTR

#define GLITE_JP_SYSTEM_NS	"http://egee.cesnet.cz/en/WSDL/jp-system"
#define GLITE_JP_ATTR_OWNER	GLITE_JP_SYSTEM_NS ":owner"

void glite_jp_attrval_free(glite_jp_attrval_t *,int);

/* Search through registered type plugins and call appropriate plugin method.
 * See type_plugin.h for detailed description.
 */

int glite_jp_attrval_cmp(glite_jp_context_t ctx,const glite_jp_attrval_t *a,const glite_jp_attrval_t *b,int *result);

char *glite_jp_attrval_to_db_full(glite_jp_context_t ctx,const glite_jp_attrval_t *attr);
char *glite_jp_attrval_to_db_index(glite_jp_context_t ctx,const glite_jp_attrval_t *attr,int len);

int glite_jp_attrval_from_db(glite_jp_context_t ctx,const char *str,glite_jp_attrval_t *attr);
const char *glite_jp_attrval_db_type_full(glite_jp_context_t ctx,const char *attr);
const char *glite_jp_attrval_db_type_index(glite_jp_context_t ctx,const char *attr,int len);



#endif
