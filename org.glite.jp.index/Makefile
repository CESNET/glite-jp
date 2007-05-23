# defaults
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
jpprefix=jp
package=glite-lb-server
version=0.0.0
PREFIX=/opt/glite

glite_location=/opt/glite
globus_prefix=/opt/globus
nothrflavour=gcc32
thrflavour=gcc32pthr
gsoap_prefix=/software/gsoap-2.6

CC=gcc

-include Makefile.inc


VPATH=${top_srcdir}/src:${top_srcdir}/examples:${top_srcdir}/project:${top_srcdir}/doc:${jpproject}:${stagedir}/interface

GLOBUS_LIBS:=-L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} \
	-lglobus_gssapi_gsi_${nothrflavour}

GLOBUS_CFLAGS:=-I${globus_prefix}/include/${nothrflavour}

DEBUG:=-W -Wall -g -O0
CPPFLAGS:=-DDEBUG -D_GNU_SOURCE -I. -I${top_srcdir}/interface -I${top_srcdir}/src -I${gsoap_prefix}/include -I${gsoap_prefix} -I${stagedir}/include ${GLOBUS_CFLAGS} -I${gsoap_prefix}/include
CFLAGS:=${DEBUG} ${CFLAGS}
LDFLAGS:=-L${stagedir}/lib

dotless_soap_ver:=${shell echo ${gsoap_version} | tr -d . }
GSOAPLIB:=-L${stagedir}/lib -lglite_security_gsoap_plugin_${dotless_soap_ver}_${nothrflavour}

gsoap_bin_prefix:=${shell if [ -x  ${gsoap_prefix}/bin/soapcpp2 ]; then echo ${gsoap_prefix}/bin; else echo ${gsoap_prefix}; fi }


LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install


daemon:=glite-jp-indexd
examples:=glite-jpis-test glite-jpis-client
test:=run-test.sh
test_files:=dump1.sql simple_query.in simple_query.out complex_query.in complex_query.out authz.out jobid_query.in jobid_query.out origin_query.in origin_query.out exists_query.in exists_query.out within_query.in within_query.out

MANS1:=glite-jpis-client.1
MANS8:=glite-jp-indexd.8
MANS:=${MANS1} ${MANS8}
HTMLS:=glite-jpis-client.html glite-jp-indexd.html
ws_prefix:=jp_
is_prefix:=${ws_prefix}
ps_prefix:=${ws_prefix}

SRCS:= conf.c bones_server.c soap_ops.c soap_ps_calls.c common.c \
	${is_prefix}Server.c ${ps_prefix}Client.c ${ws_prefix}C.c \
	ws_ps_typeref.c ws_is_typeref.c db_ops.c context.c common_server.c 

EXA_TEST_SRCS:=jpis-test.c ${is_prefix}Client.c ${is_prefix}C.c context.c db_ops.c conf.c ws_is_typeref.c
EXA_DB_SRCS:=jpis-db-internal.c db_ops.c conf.c context.c ws_is_typeref.c
EXA_CLIENT_SRCS:=jpis-client.c ${is_prefix}Client.c ${is_prefix}C.c common.c

OBJS:=${SRCS:.c=.o}
EXA_TEST_OBJS:=${EXA_TEST_SRCS:.c=.o}
EXA_DB_OBJS:=${EXA_DB_SRCS:.c=.o}
EXA_CLIENT_OBJS:=${EXA_CLIENT_SRCS:.c=.o}


COMMONLIB:=-lglite_jp_common_${nothrflavour}
SRVCOMMONLIB:=-lglite_jp_server_common
BONESLIB:=-lglite_lb_server_bones
TRIOLIB:=-lglite_jp_trio

default all: compile doc

compile: ${daemon} ${examples}

${daemon}: ${OBJS}
	${LINK} -o $@ -export-dynamic ${OBJS} ${BONESLIB} ${TRIOLIB} ${COMMONLIB} ${GSOAPLIB} ${SRVCOMMONLIB}

glite-jpis-test: ${EXA_TEST_OBJS}
	${LINK} -o $@ $+ ${GSOAPLIB} ${COMMONLIB} ${TRIOLIB} ${SRVCOMMONLIB}

jpis-db-internal: ${EXA_DB_OBJS}
	${LINK} -o $@ $+ ${COMMONLIB} ${SRVCOMMONLIB}

glite-jpis-client: ${EXA_CLIENT_OBJS}
	${LINK} -o $@ $+ ${GSOAPLIB} ${COMMONLIB} ${TRIOLIB}

${ws_prefix}Client.c ${ws_prefix}Server.c \
${ws_prefix}C.c ${ws_prefix}H.h ${ws_prefix}Stub.h: JobProvenance.xh
	${gsoap_bin_prefix}/soapcpp2 -n -w -c -p ${ws_prefix} $<

JobProvenance.xh: jpdev.wsdl
	cp ${stagedir}/interface/JobProvenanceTypes.wsdl .
	${gsoap_bin_prefix}/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o $@ $<
	rm -f JobProvenanceTypes.wsdl

check:
	# ../test/run-test.sh

doc: ${MANS} ${HTMLS}

stage: compile doc
	${MAKE} PREFIX=${stagedir} DOSTAGE=yes install

dist: distsrc distbin

distsrc:
	mkdir -p ${top_srcdir}/${package}-${version}
	cd ${top_srcdir} && GLOBIGNORE="${package}-${version}" && cp -Rf * ${package}-${version}
	cd ${top_srcdir} && tar -czf ${distdir}/${package}-${version}_src.tar.gz --exclude-from=project/tar_exclude ${package}-${version}
	rm -rf ${top_srcdir}/${package}-${version}

distbin:
	$(MAKE) install PREFIX=`pwd`/tmpbuilddir${stagedir}
	save_dir=`pwd`; cd tmpbuilddir${stagedir} && tar -czf $$save_dir/${top_srcdir}/${distdir}/${package}-${version}_bin.tar.gz *; cd $$save_dir
	rm -rf tmpbuilddir
	
install:
	-mkdir -p ${PREFIX}/bin ${PREFIX}/etc ${PREFIX}/etc/init.d
	-mkdir -p ${PREFIX}/share/doc/${package}-${version} 
	-mkdir -p ${PREFIX}/examples ${PREFIX}/examples/query-tests
	-mkdir -p ${PREFIX}/interface
	-mkdir -p ${PREFIX}/share/man/man1
	-mkdir -p ${PREFIX}/share/man/man8
	${INSTALL} -m 755 ${daemon} ${PREFIX}/bin
	${INSTALL} -m 755 ${examples} ${PREFIX}/examples
	${INSTALL} -m 755 ${top_srcdir}/config/startup ${PREFIX}/etc/init.d/glite-jp-indexd
	${INSTALL} -m 755 ${top_srcdir}/config/glite-jp-index-dbsetup.sql ${PREFIX}/etc
	${INSTALL} -m 755 ${top_srcdir}/config/glite-jpis-config.xml ${PREFIX}/etc
	${INSTALL} -m 755 ${top_srcdir}/config/glite-jpis-test-config.xml ${PREFIX}/etc
	${INSTALL} -m 644 ${top_srcdir}/doc/README ${HTMLS} ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${MANS1} ${PREFIX}/share/man/man1
	${INSTALL} -m 644 ${MANS8} ${PREFIX}/share/man/man8
	${INSTALL} -m 755 ../examples/query-tests/${test} ${PREFIX}/examples/query-tests
	for i in ${test_files}; do \
		${INSTALL} -m 644 ../examples/query-tests/$$i ${PREFIX}/examples/query-tests; \
	done
	${INSTALL} -m 644 ${top_srcdir}/interface/JobProvenanceISClient.xsd ${PREFIX}/interface

clean:

soap_ops.o bones_server.o simple_server.o: ${is_prefix}H.h ${ps_prefix}H.h

soap_ops.o bones_server.o: soap_version.h

soap_version.h:
	${gsoap_bin_prefix}/soapcpp2 /dev/null
	perl -ne '$$. == 2 && /.*([0-9])\.([0-9])\.([0-9]).*/ && printf "#define GSOAP_VERSION %d%02d%02d\n",$$1,$$2,$$3' soapH.h >$@
	-rm soapC.cpp soapH.h soapStub.h soapClient.cpp soapServer.cpp soapClientLib.cpp soapServerLib.cpp

db_ops.h: context.h
context.h: conf.h
db_ops.o: db_ops.c conf.h context.h db_ops.h
context.o: context.c conf.h context.h
soap_ps_calls.o: soap_ps_calls.c ${ps_prefix}H.h ${ps_prefix}.nsmap soap_version.h conf.h db_ops.h ws_ps_typeref.h context.h
soap_ops.o: soap_ops.c jp_H.h ${is_prefix}.nsmap soap_version.h db_ops.h ws_ps_typeref.h ws_is_typeref.h context.h
ws_ps_typeref.o: ws_ps_typeref.c ${ps_prefix}H.h ws_typemap.h ws_ps_typeref.h soap_version.h
ws_is_typeref.o: ws_is_typeref.c ${is_prefix}H.h ws_typemap.h ws_is_typeref.h soap_version.h
comon_server.o: common_server.c common_server.h
jpis-client.o: jpis-client.c ${is_prefix}H.h soap_version.h
jpis-test.o: jpis-client.c ${is_prefix}H.h soap_version.h
conf.o: conf.c ${is_prefix}H.h soap_version.h

${ws_prefix}C.o: ${ws_prefix}C.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -Wno-unused-parameter $<

%.1: %.sgml
	docbook2man $<

%.8: %.sgml
	docbook2man $<

%.html: %.sgml
	docbook2html $< --nochunks

.PHONY: default all compile check doc stage dist distsrc distbin install clean
