#ifndef __GLITE_JP_FILEPLUGIN
#define __GLITE_JP_FILEPLUGIN

/** Methods of the file plugin. */

typedef struct _glite_jp_fplug_op_t {

/** Open a file.
\param[in] fpctx	Context of the plugin, returned by its init.
\param[in] bhandle	Handle of the file via JPPS backend.
\param[out] handle	Handle to the opened file structure, to be passed to other plugin functions.
*/
	int	(*open)(void *fpctx,void *bhandle,void **handle);

/** Close the file. Free data associated to a handle */
	int	(*close)(void *fpctx,void *handle);

/** Retrieve value(s) of an attribute.
\param[in] fpctx	Plugin context.
\param[in] handle	Handle of the opened file.
\param[in] attr		Queried attribute.
\param[out] attrval	GLITE_JP_ATTR_UNDEF-terminated list of value(s) of the attribute.
			If there are more and there is an interpretation of their order
			they must be sorted, eg. current value of tag is the last one.
\retval	0 success
\retval ENOSYS	this attribute is not defined by this type of file
\retval ENOENT	no value is present 
*/
	int	(*attr)(void *fpctx,void *handle,glite_jp_attr_t attr,glite_jp_attrval_t **attrval);

/** File type specific operation. 
\param[in] fpctx	Plugin context.
\param[in] handle	Handle of the opened file.
\param[in] oper		Code of the operation, specific for a concrete plugin.
*/
	int	(*op)(void *fpctx,void *handle,int oper,...);
	
} glite_jp_fplug_op_t;

/** Initialisation function of the plugin. 
  Called after dlopen(), must be named "init".
\param[in] ctx		JPPS context
\param[out] uris	NULL-terminated list of file types (URIs) handled by the plugin
\param[out] ops		Plugin methods.
\param[out] fpctx	Initialised plugin context, to be passed to the methods.
*/
  
typedef int (*glite_jp_fplug_init_t)(glite_jp_context_t ctx,char ***uris,glite_jp_fplug_op_t *ops,void **fpctx);


#endif
