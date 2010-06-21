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

