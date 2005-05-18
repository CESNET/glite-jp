#include "soap_version.h"

#if GSOAP_VERSION >= 20700
#define INPUT_SANDBOX	jptype__UploadClass__INPUT_SANDBOX
#define OUTPUT_SANDBOX	jptype__UploadClass__OUTPUT_SANDBOX
#define JOB_LOG	jptype__UploadClass__JOB_LOG

#define OWNER jptype__AttributeType__OWNER
#define TIME jptype__AttributeType__TIME
#define TAG jptype__AttributeType__TAG

#define EQUAL jptype__queryOp__EQUAL
#define UNEQUAL jptype__queryOp__UNEQUAL
#define LESS jptype__queryOp__LESS
#define GREATER jptype__queryOp__GREATER
#define WITHIN jptype__queryOp__WITHIN
#endif

