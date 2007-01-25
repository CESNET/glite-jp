#ifndef __GLITE_JP_KNOWN_ATTR
#define __GLITE_JP_KNOWN_ATTR

/** Namespace of JP system attributes */
#define GLITE_JP_SYSTEM_NS	"http://egee.cesnet.cz/en/Schema/JP/System"
#define GLITE_JP_WORKFLOW_NS	"http://egee.cesnet.cz/en/Schema/JP/Workflow"

/** Job owner, as specified with RegisterJob JPPS operation */
#define GLITE_JP_ATTR_OWNER	GLITE_JP_SYSTEM_NS ":owner" 

/** JobId */
#define GLITE_JP_ATTR_JOBID	GLITE_JP_SYSTEM_NS ":jobId" 

/** Timestamp of job registration in JP.
 * Should be almost the same time as registration with LB. */
#define GLITE_JP_ATTR_REGTIME	GLITE_JP_SYSTEM_NS ":regtime" 

/** Workflow node relationships. */
#define GLITE_JP_ATTR_WF_ANCESTOR GLITE_JP_WORKFLOW_NS ":ancestor"
#define GLITE_JP_ATTR_WF_SUCCESSOR GLITE_JP_WORKFLOW_NS ":successor"

/** Attributes derived from LB system data
 * \see jp_job_attrs.h */

/** Namespace for LB user tags, schemaless, all values are strings */
#define GLITE_JP_LBTAG_NS	"http://egee.cesnet.cz/en/WSDL/jp-lbtag"
#define GLITE_JP_JDL_NS 	"http://jdl"

/** Namespace for Sandboxes */
#define GLITE_JP_ISB_NS       "http://egee.cesnet.cz/en/Schema/JP/ISB"
#define GLITE_JP_OSB_NS       "http://egee.cesnet.cz/en/Schema/JP/OSB"

/** File names in SB tar */
#define GLITE_JP_ATTR_ISB_FILENAME     GLITE_JP_ISB_NS ":fileName"
#define GLITE_JP_ATTR_OSB_FILENAME     GLITE_JP_OSB_NS ":fileName"

#endif
