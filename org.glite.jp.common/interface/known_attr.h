#ifndef __GLITE_JP_KNOWN_ATTR
#define __GLITE_JP_KNOWN_ATTR

/** Namespace of JP system attributes */
#define GLITE_JP_SYSTEM_NS	"http://egee.cesnet.cz/en/WSDL/jp-system"

/** Job owner, as specified with RegisterJob JPPS operation */
#define GLITE_JP_ATTR_OWNER	GLITE_JP_SYSTEM_NS ":owner" 

/** Timestamp of job registration in JP.
 * Should be almost the same time as registration with LB. */
#define GLITE_JP_ATTR_REGTIME	GLITE_JP_SYSTEM_NS ":regtime" 

/** Namespace for attributes derived from LB system data */
#define GLITE_JP_LB_NS		"http://egee.cesnet.cz/en/WSDL/jp-lb"

#define GLITE_JP_LB_SUBMITTED	GLITE_JP_LB_NS ":submitted"	/**< submit time */
#define GLITE_JP_LB_TERMINATED	GLITE_JP_LB_NS ":terminated"	/**< termination time (done, abort, cancel) */
#define GLITE_JP_LB_FINALSTATE	GLITE_JP_LB_NS ":finalState"	/**< final job status */
/* TODO: others */

/** Namespace for LB user tags, schemaless, all values are strings */
#define GLITE_JP_LBTAG_NS	"http://egee.cesnet.cz/en/WSDL/jp-lbtag"

#endif
