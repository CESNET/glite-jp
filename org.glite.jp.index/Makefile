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
ares_prefix=/opt/ares
gsoap_prefix=/software/gsoap-2.6

CC=gcc

-include Makefile.inc


VPATH=${top_srcdir}/src:${top_srcdir}/examples:${top_srcdir}/test:${top_srcdir}/project:${jpproject}:${stagedir}/interface

GLOBUS_LIBS:=-L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} \
	-lglobus_gssapi_gsi_${nothrflavour}

GLOBUS_CFLAGS:=-I${globus_prefix}/include/${nothrflavour}

CPPFLAGS:=-DDEBUG -D_GNU_SOURCE
DEBUG:=-W -Wall -Wno-unused-parameter -Wno-unused-function -g -O0

dotless_soap_ver:=${shell echo ${gsoap_version} | tr -d . }
GSOAPLIB:=-L${stagedir}/lib -lglite_security_gsoap_plugin_${dotless_soap_ver}_${nothrflavour} -L${ares_prefix}/lib -lares

gsoap_bin_prefix:=${shell if [ -x  ${gsoap_prefix}/bin/soapcpp2 ]; then echo ${gsoap_prefix}/bin; else echo ${gsoap_prefix}; fi }

CFLAGS:=${CPPFLAGS} ${DEBUG} -I. -I${top_srcdir}/interface -I${top_srcdir}/src -I${gsoap_prefix}/include -I${gsoap_prefix} -I${stagedir}/include ${GLOBUS_CFLAGS} -I${mysql_prefix}/include -I${mysql_prefix}/include/mysql
LDFLAGS:=-L${stagedir}/lib

LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install


daemon:=glite-jp-indexd
example:=jpis-test
is_prefix:=jpis_
ps_prefix:=jpps_

SRCS:= conf.c bones_server.c soap_ops.c soap_ps_calls.c \
	ws_ps_typeref.c ws_is_typeref.c db_ops.c context.c \
	${is_prefix}ServerLib.c \
	${ps_prefix}ClientLib.c ${ps_prefix}C.c \

EXA_SRCS:=jpis-test.c ${is_prefix}C.c ${is_prefix}Client.c context.c

OBJS:=${SRCS:.c=.o}
EXA_OBJS:=${EXA_SRCS:.c=.o}


COMMONLIB:=-lglite_jp_common
SRVCOMMONLIB:=-lglite_jp_server_common
BONESLIB:=-lglite_lb_server_bones
TRIOLIB:=-lglite_lb_trio

ifneq (${mysql_prefix},/usr)
	ifeq ($(shell echo ${mysql_version} | cut -d. -f1,2),4.1)
		MYSQLIB := -L${mysql_prefix}/lib/mysql -lmysqlclient
	else
		MYSQLIB := -L${mysql_prefix}/lib -lmysqlclient
	endif
else
	MYSQLIB := -lmysqlclient
endif


default all: compile

compile: ${daemon} ${example}

${daemon}: ${OBJS}
	${LINK} -o $@ -export-dynamic ${OBJS} ${BONESLIB} ${TRIOLIB} ${COMMONLIB} ${GSOAPLIB} ${GLOBUS_LIBS} ${MYSQLIB} ${SRVCOMMONLIB}

${example}: ${EXA_OBJS}
	${LINK} -o $@ ${EXA_OBJS} ${GSOAPLIB} ${GLOBUS_LIBS}

jpis-db-internal: jpis-db-internal.o db_ops.o conf.o context.o
	${LINK} -o $@ $+ ${COMMONLIB} ${SRVCOMMONLIB} ${GLOBUS_LIBS}

JobProvenanceIS.xh: JobProvenanceIS.wsdl JobProvenanceTypes.wsdl typemap.dat
	cp  ${stagedir}/interface/JobProvenanceTypes.wsdl .
	${gsoap_bin_prefix}/wsdl2h -t ${top_srcdir}/src/typemap.dat -c -o $@ $<
	rm -f JobProvenanceTypes.wsdl

JobProvenancePS.xh: %.xh: %.wsdl JobProvenanceTypes.wsdl typemap.dat
	cp  ${stagedir}/interface/JobProvenanceTypes.wsdl .
	${gsoap_bin_prefix}/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o $@ $<
	rm -f JobProvenanceTypes.wsdl

${is_prefix}ClientLib.c ${is_prefix}Client.c \
${is_prefix}Server.c ${is_prefix}ServerLib.c  \
${is_prefix}C.c ${is_prefix}H.h: JobProvenanceIS.xh
	${gsoap_bin_prefix}/soapcpp2 -n -w -c -p ${is_prefix} JobProvenanceIS.xh

${ps_prefix}Client.c ${ps_prefix}ClientLib.c  \
${ps_prefix}C.c ${ps_prefix}H.h: JobProvenancePS.xh
	${gsoap_bin_prefix}/soapcpp2 -n -w -c -p ${ps_prefix} JobProvenancePS.xh


#env_C.c env_Server.c:
#	touch env.xh
#	cp  ${stagedir}/interface/JobProvenanceTypes.wsdl .
#	${gsoap_prefix}/bin/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o env.xh JobProvenanceTypes.wsdl
#	rm -f JobProvenanceTypes.wsdl
#	${gsoap_prefix}/bin/soapcpp2 -w -c -p env_ env.xh

check: 
	-echo nothing yet

doc:

stage: compile
	${INSTALL} -m 755 ${daemon} ${stagedir}/bin

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
	${INSTALL} -m 755 glite-jp-indexd ${PREFIX}/bin; \

#	${INSTALL} -m 755 ${top_srcdir}/config/startup ${PREFIX}/etc/init.d/glite-jp-indexd

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

.PHONY: default all compile check doc stage dist distsrc distbin install clean
