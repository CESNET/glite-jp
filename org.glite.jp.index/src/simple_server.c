#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "jpis_H.h"

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
