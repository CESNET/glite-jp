#ifndef __GLITE_JP_TYPEPLUGIN
#define __GLITE_JP_TYPEPLUGIN

typedef struct _glite_jp_tplug_data_t {
	
	char	*namespace;
	void	*pctx;

/** Compare attribute values. 
  * \param[in] a value to compare
  * \param[in] b value to compare
  * \param[out] result like strcmp()
  * \param[out] err set if the values cannot be compared
  * \retval 0 OK
  * \retval other error
  */
	int (*cmp)(
		void	*ctx,
		const glite_jp_attrval_t *a,
		const glite_jp_attrval_t *b,
		int	*result);

/** Convert to and from XML representation */
	char (*to_xml)(void *,const glite_jp_attrval_t *a);
	glite_jp_attrval_t (*from_xml)(void *,const char *,const char *);

/** Convert to and from database string representation */
	char (*to_db)(void *,const glite_jp_attrval_t *a);
	glite_jp_attrval_t (*from_db)(void *,const char *);

/** Query for database type. 
 * Useful for db column dynamic creation etc.
 */
	const char (*db_type)(void *,const glite_jp_attr_t *);

} glite_jp_tplug_data_t;

/** Plugin init function.
    Must be called init, supposed to be called as many times as required
    for different param's (e.g. xsd files).
 */

typedef int (*glite_jp_tplug_init_t)(
	glite_jp_context_t	ctx,
	const char		*param,
	glite_jp_tplug_data	*plugin_data
);

#endif
