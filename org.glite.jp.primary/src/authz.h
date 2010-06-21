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

