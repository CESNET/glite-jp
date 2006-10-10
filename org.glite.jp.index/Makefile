# defaults
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
lbprefix=lb
package=glite-lb-server
version=0.0.0
PREFIX=/opt/glite

glite_location=/opt/glite
globus_prefix=/opt/globus
nothrflavour=gcc32
thrflavour=gcc32pthr
expat_prefix=/opt/expat
gsoap_prefix=/software/gsoap-2.6

CC=gcc

-include Makefile.inc


VPATH=${top_srcdir}/src:${top_srcdir}/examples:${top_srcdir}/project:${top_srcdir}/doc:${jpproject}:${stagedir}/interface

GLOBUS_LIBS:=-L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} \
	-lglobus_gssapi_gsi_${nothrflavour}

GLOBUS_CFLAGS:=-I${globus_prefix}/include/${nothrflavour}

DEBUG:=-W -Wall -Wno-unused-parameter -Wno-unused-function -g -O0
CPPFLAGS:=-DDEBUG -D_GNU_SOURCE -I. -I${top_srcdir}/interface -I${top_srcdir}/src -I${gsoap_prefix}/include -I${gsoap_prefix} -I${stagedir}/include ${GLOBUS_CFLAGS} -I${mysql_prefix}/include -I${mysql_prefix}/include/mysql -I${gsoap_prefix}/include
CFLAGS:=${DEBUG}
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
test_files:=dump1.sql simple_query.in simple_query.out complex_query.in complex_query.out authz.out 
MANS1:=glite-jpis-client.1
MANS8:=glite-jp-indexd.8
MANS:=${MANS1} ${MANS8}
HTMLS:=glite-jpis-client.html glite-jp-indexd.html
is_prefix:=jpis_
is_client_prefix:=jpis_client_
ps_prefix:=jpps_

SRCS:= conf.c bones_server.c soap_ops.c soap_ps_calls.c \
	ws_ps_typeref.c ws_is_typeref.c db_ops.c context.c common_server.c \
<<<<<<< Makefile
	${is_prefix}Server.c ${is_prefix}C.c\
	${ps_prefix}ClientLib.c
=======
	${is_prefix}ServerLib.c \
	${ps_prefix}ClientLib.c ${ps_prefix}C.c
>>>>>>> 1.32.2.3

<<<<<<< Makefile
EXA_TEST_SRCS:=jpis-test.c ${is_prefix}C.c ${is_prefix}Client.c context.c db_ops.c conf.c ws_is_typeref.c
EXA_DB_SRCS:=jpis-db-internal.c db_ops.c conf.c context.c ws_is_typeref.c
EXA_CLIENT_SRCS:=jpis-client.c common.c \
	${is_client_prefix}C.c ${is_client_prefix}Client.c
=======
EXA_TEST_SRCS:=jpis-test.c ${is_prefix}C.c ${is_prefix}Client.c context.c db_ops.c conf.c
EXA_DB_SRCS:=jpis-db-internal.c db_ops.c conf.c context.c
EXA_CLIENT_SRCS:=jpis-client.c common.c \
	${is_client_prefix}C.c ${is_client_prefix}Client.c
>>>>>>> 1.32.2.3
#                ${is_prefix}C.c

OBJS:=${SRCS:.c=.o}
EXA_TEST_OBJS:=${EXA_TEST_SRCS:.c=.o}
EXA_DB_OBJS:=${EXA_DB_SRCS:.c=.o}
EXA_CLIENT_OBJS:=${EXA_CLIENT_SRCS:.c=.o}


COMMONLIB:=-lglite_jp_common
SRVCOMMONLIB:=-lglite_jp_server_common
BONESLIB:=-lglite_lb_server_bones
TRIOLIB:=-lglite_jp_trio

default all: compile doc

compile: ${daemon} ${examples} copy_tests

${daemon}: ${OBJS}
	${LINK} -o $@ -export-dynamic ${OBJS} ${BONESLIB} ${TRIOLIB} ${COMMONLIB} ${GSOAPLIB} ${GLOBUS_LIBS} ${SRVCOMMONLIB}

glite-jpis-test: ${EXA_TEST_OBJS}
	${LINK} -o $@ $+ ${GSOAPLIB} ${GLOBUS_LIBS} ${COMMONLIB} ${TRIOLIB} ${SRVCOMMONLIB}

jpis-db-internal: ${EXA_DB_OBJS}
	${LINK} -o $@ $+ ${COMMONLIB} ${SRVCOMMONLIB} ${GLOBUS_LIBS}

glite-jpis-client: ${EXA_CLIENT_OBJS}
	${LINK} -o $@ $+ ${GSOAPLIB}

copy_tests: ${test} ${test_files}

${test} ${test_files}:
	for i in $@; do cp ${top_srcdir}/examples/query-tests/$$i  ${top_srcdir}/build; done

JobProvenanceIS.xh: JobProvenanceIS.wsdl JobProvenanceTypes.wsdl typemap.dat
	cp  ${stagedir}/interface/JobProvenanceTypes.wsdl .
	${gsoap_bin_prefix}/wsdl2h -t ${top_srcdir}/src/typemap.dat -c -o $@ $<
	rm -f JobProvenanceTypes.wsdl

JobProvenancePS.xh: %.xh: %.wsdl JobProvenanceTypes.wsdl typemap.dat
	cp  ${stagedir}/interface/JobProvenanceTypes.wsdl .
	${gsoap_bin_prefix}/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o $@ $<
	rm -f JobProvenanceTypes.wsdl

#JobProvenanceISClient.xh: typemap.dat JobProvenanceISClient.xsd JobProvenanceIS.wsdl
#	cp  ${stagedir}/interface/JobProvenance{Types.xsd,Types.wsdl,IS.wsdl} .
#	${gsoap_bin_prefix}/wsdl2h -t ${top_srcdir}/src/typemap.dat -c -o $@ JobProvenanceIS.wsdl ${top_srcdir}/src/JobProvenanceISClient.xsd
#	rm -f JobProvenance{Types.xsd,Types.wsdl,IS.wsdl}

${is_prefix}ClientLib.c ${is_prefix}Client.c \
${is_prefix}Server.c ${is_prefix}ServerLib.c  \
${is_prefix}C.c ${is_prefix}H.h ${is_prefix}_Stub.h: JobProvenanceIS.xh
	${gsoap_bin_prefix}/soapcpp2 -n -w -c -p ${is_prefix} $<

# JobProvenanceISClient.xh should be used, 
# for now (gsoap-2.7.0) used direct JobProvenanceIS.xh
${is_client_prefix}ClientLib.c ${is_client_prefix}Client.c \
${is_client_prefix}Server.c ${is_client_prefix}ServerLib.c  \
${is_client_prefix}C.c ${is_client_prefix}H.h ${is_client_prefix}_Stub.h: JobProvenanceIS.xh
	${gsoap_bin_prefix}/soapcpp2 -n -w -c -p ${is_client_prefix} $<

${ps_prefix}Client.c ${ps_prefix}ClientLib.c  \
${ps_prefix}C.c ${ps_prefix}H.h ${ps_prefix}_Stub.h: JobProvenancePS.xh
	${gsoap_bin_prefix}/soapcpp2 -n -w -c -p ${ps_prefix} $<


#env_C.c env_Server.c:
#	touch env.xh
#	cp  ${stagedir}/interface/JobProvenanceTypes.wsdl .
#	${gsoap_prefix}/bin/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o env.xh JobProvenanceTypes.wsdl
#	rm -f JobProvenanceTypes.wsdl
#	${gsoap_prefix}/bin/soapcpp2 -w -c -p env_ env.xh

check:
	# ../test/run-test.sh

doc: ${MANS} ${HTMLS}

stage: compile
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
	${INSTALL} -m 644 ${top_srcdir}/doc/README ${HTMLS} ${PREFIX}/share/doc/${package}-${version}
	${INSTALL} -m 644 ${MANS1} ${PREFIX}/share/man/man1
	${INSTALL} -m 644 ${MANS8} ${PREFIX}/share/man/man8
	${INSTALL} -m 755 ${test} ${PREFIX}/examples/query-tests
	${INSTALL} -m 644 ${test_files} ${PREFIX}/examples/query-tests
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
soap_ps_calls.o: soap_ps_calls.c jpps_H.h jpps_.nsmap soap_version.h conf.h db_ops.h ws_ps_typeref.h context.h
soap_ops.o: soap_ops.c jpis_H.h jpis_.nsmap soap_version.h db_ops.h ws_ps_typeref.h ws_is_typeref.h context.h
ws_ps_typeref.o: ws_ps_typeref.c jpis_H.h ws_typemap.h ws_ps_typeref.h
ws_is_typeref.o: ws_is_typeref.c jpis_H.h ws_typemap.h ws_is_typeref.h
comon_server.o: common_server.c common_server.h
jpis-client.o: jpis-client.c ${is_client_prefix}H.h soap_version.h
conf.o: conf.c ${is_prefix}H.h soap_version.h

%.1: %.sgml
	docbook2man $<

%.8: %.sgml
	docbook2man $<

%.html: %.sgml
	docbook2html $< --nochunks

.PHONY: default all compile check doc stage dist distsrc distbin install clean
