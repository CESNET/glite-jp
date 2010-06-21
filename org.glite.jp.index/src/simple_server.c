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

#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "jp_H.h"

int main() {
   struct soap soap;
   int i, m, s; // master and slave sockets

   glite_jp_context_t	ctx;

   soap_init(&soap);
   glite_jp_init_context(&ctx);
   soap.user = (void *) ctx;

   srand48(time(NULL)); /* feed id generation */

   m = soap_bind(&soap, NULL, 8902, 100);
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
         soap_serve(&soap); // process RPC request
         soap_destroy(&soap); // clean up class instances
         soap_end(&soap); // clean up everything and close socket
	 glite_jp_run_deferred(ctx);
      }
   }
   soap_done(&soap); // close master socket

   return 0;
}
