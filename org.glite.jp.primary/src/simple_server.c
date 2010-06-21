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

#include <stdio.h>

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "jpps_H.h"

extern SOAP_NMAC struct Namespace jpis__namespaces[],jpps__namespaces[];

int main(int argc, char *argv[]) {
   struct soap soap;
   int i, m, s; // master and slave sockets

   glite_jp_context_t	ctx;

   soap_init(&soap);
   soap_set_namespaces(&soap, jpps__namespaces);

   glite_jp_init_context(&ctx);

   if (glite_jppsbe_init(ctx, &argc, argv)) {
	   /* XXX log */
	   fputs(glite_jp_error_chain(ctx), stderr);
	   exit(1);
   }

   soap.user = (void *) ctx;

   ctx->other_soap = soap_new();
   soap_init(ctx->other_soap);
   soap_set_namespaces(ctx->other_soap,jpis__namespaces);

   srand48(time(NULL)); /* feed id generation */

   m = soap_bind(&soap, NULL, 8901, 100);
   if (m < 0)
      soap_print_fault(&soap, stderr);
   else
   {
      fprintf(stderr, "Socket connection successful: master socket = %d\n", m);
      for (i = 1; ; i++) {
         s = soap_accept(&soap);
         if (s < 0) {
            soap_print_fault(&soap, stderr);
            break;
         }
         jpps__serve(&soap); // process RPC request
         soap_destroy(&soap); // clean up class instances
         soap_end(&soap); // clean up everything and close socket
	 glite_jp_run_deferred(ctx);
      }
   }
   soap_done(&soap); // close master socket

   return 0;
}

/* XXX: we don't use it */
SOAP_NMAC struct Namespace namespaces[] = { {NULL,NULL} };
