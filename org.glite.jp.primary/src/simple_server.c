#include "glite/jp/types.h"
#include "glite/jp/context.h"

#include "jpps_H.h"

extern SOAP_NMAC struct Namespace jpis__namespaces[],jpps__namespaces[];

int main() {
   struct soap soap;
   int i, m, s; // master and slave sockets

   glite_jp_context_t	ctx;

   soap_init(&soap);
   soap_set_namespaces(&soap, jpps__namespaces);

   glite_jp_init_context(&ctx);
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
