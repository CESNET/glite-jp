/**
 * Check authorisation of JPPS operation on job.
 *
 * \param[in] ctx	JP context including peer name & other credentials (VOMS etc.)
 * \param[in] op	operation, one of SOAP_TYPE___jpsrv__*
 * \param[in] job	jobid of the job to decide upon
 * \param[in] owner	current known owner of the job (may be NULL), shortcut to avoid
 *			unnecessary database query.
 *
 * \retval 0		OK, operation permitted
 * \retval EPERM	denied
 * \retval other	error
 */

int glite_jpps_authz(glite_jp_context_t ctx,int op,const char *job,const char *owner);

int glite_jpps_readauth(glite_jp_context_t ctx,const char *file);

