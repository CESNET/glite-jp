# defaults
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
jpprefix=jp
package=glite-jp-primary
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


VPATH=${top_srcdir}/src:${top_srcdir}/examples:${top_srcdir}/test:${top_srcdir}/project:${stagedir}/interface

GLOBUS_LIBS:=-L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} \
	-lglobus_gssapi_gsi_${nothrflavour}

GLOBUS_CFLAGS:=-I${globus_prefix}/include/${nothrflavour}

DEBUG:=-g -O0  -DDEBUG

CFLAGS:=${DEBUG} -I. -I${top_srcdir}/interface -I${top_srcdir}/src -I${gsoap_prefix}/include -I${gsoap_prefix} -I${stagedir}/include ${GLOBUS_CFLAGS} -I${mysql_prefix}/include -I${mysql_prefix}/include/mysql
LDFLAGS:=-L${stagedir}/lib

LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LTCOMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
SOLINK:=libtool --mode=link ${CC} -module ${LDFLAGS} -rpath ${stagedir}/lib
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

daemon:=glite-jp-primarystoraged
example:=jpps-test
ps_prefix:=jpps_
is_prefix:=jpis_

plugins:=glite-jp-tags.la glite-jp-ftpdauth.la

HDRS_I=file_plugin.h
HDRS_S=builtin_plugins.h backend.h

SRCS:= bones_server.c soap_ops.c \
	new_ftp_backend.c mysql.c file_plugin.c \
	feed.c authz.c attrs.c\
	is_client.c \
	env_C.c

#	${ps_prefix}ServerLib.c \
#	${is_prefix}ClientLib.c jpps_C.c

EXA_SRCS:=jpps-test.c ${ps_prefix}C.c ${ps_prefix}Client.c

gsoap_bin_prefix:=${shell if [ -x  ${gsoap_prefix}/bin/soapcpp2 ]; then echo ${gsoap_prefix}/bin; else echo ${gsoap_prefix}; fi }

OBJS:=${SRCS:.c=.o}
EXA_OBJS:=${EXA_SRCS:.c=.o}

dotless_soap_ver:=${shell echo ${gsoap_version} | tr -d . }
COMMONLIB:=-lglite_jp_common
BONESLIB:=-lglite_lb_server_bones
GSOAPLIB:=-L${stagedir}/lib -lglite_security_gsoap_plugin_${dotless_soap_ver}_${nothrflavour} 
TRIOLIB:=-lglite_jp_trio

ifneq (${mysql_prefix},/usr)
	ifeq ($(shell echo ${mysql_version} | cut -d. -f1,2),4.1)
		MYSQLIB := -L${mysql_prefix}/lib/mysql -lmysqlclient -lz
        else
		MYSQLIB := -L${mysql_prefix}/lib -lmysqlclient -lz
        endif
else
	MYSQLIB := -lmysqlclient
endif

default all: compile

compile: ${daemon} ${example} ${plugins}

${daemon}: ${OBJS}
	${LINK} -o $@ -export-dynamic ${OBJS} ${BONESLIB} ${TRIOLIB} ${COMMONLIB} ${GSOAPLIB} ${GLOBUS_LIBS} ${MYSQLIB} 

${example}: ${EXA_OBJS}
	${LINK} -o $@ ${EXA_OBJS} ${GSOAPLIB} ${GLOBUS_LIBS}

JobProvenancePS.xh: %.xh: %.wsdl JobProvenanceTypes.wsdl typemap.dat
	cp  ${stagedir}/interface/JobProvenanceTypes.wsdl .
	${gsoap_bin_prefix}/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o $@ $<
	rm -f JobProvenanceTypes.wsdl

JobProvenanceIS.xh: %.xh: %.wsdl JobProvenanceTypes.wsdl typemap.dat
	cp  ${stagedir}/interface/JobProvenanceTypes.wsdl .
	${gsoap_bin_prefix}/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o $@ $<
	rm -f JobProvenanceTypes.wsdl


${ps_prefix}Client.c ${ps_prefix}ClientLib.c  \
${ps_prefix}Server.c ${ps_prefix}ServerLib.c  \
${ps_prefix}C.c ${ps_prefix}H.h: JobProvenancePS.xh
	${gsoap_bin_prefix}/soapcpp2 -n -w -c -p ${ps_prefix} JobProvenancePS.xh

${is_prefix}ClientLib.c ${is_prefix}Client.c \
${is_prefix}C.c ${is_prefix}H.h: JobProvenanceIS.xh
	${gsoap_bin_prefix}/soapcpp2 -n -w -c -p ${is_prefix} JobProvenanceIS.xh

soap_ops.o: ${ps_prefix}ServerLib.c soap_util.c

is_client.o: ${is_prefix}ClientLib.c soap_util.c

env_C.c env_Server.c:
	touch env.xh
	${gsoap_bin_prefix}/soapcpp2 -w -c -p env_ env.xh

#$(SOAP_PREFIX)H.h $(SOAP_PREFIX)C.c: LB.xh
#        $(GSOAP_BIN_PATH)/soapcpp2 -w -c -p $(SOAP_PREFIX) LB.xh
#
#LB.xh: LB.wsdl typemap.dat
#         $(GSOAP_BIN_PATH)/wsdl2h -c -o $@ LB.wsdl
#


bones_server.o simple_server.o: ${is_prefix}H.h ${ps_prefix}H.h

check: 
	-echo nothing yet

doc:

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
	-mkdir -p ${PREFIX}/bin ${PREFIX}/etc ${PREFIX}/examples ${PREFIX}/etc/init.d
	${INSTALL} -m 755 ${daemon} ${PREFIX}/bin
	${INSTALL} -m 755 jpps-test ${PREFIX}/examples/glite-jp-primary-test
	if [ x${DOSTAGE} = xyes ]; then \
		mkdir -p ${PREFIX}/include/${globalprefix}/${jpprefix} ; \
		(cd ${top_srcdir}/interface && install -m 644 ${HDRS_I} ${PREFIX}/include/${globalprefix}/${jpprefix}) ; \
		(cd ${top_srcdir}/src && install -m 644 ${HDRS_S} ${PREFIX}/include/${globalprefix}/${jpprefix}) ; \
	fi
	for plugin in ${plugins}; do \
		${INSTALL} -m 755 $$plugin ${PREFIX}/lib; \
	done
	${INSTALL} -m 755 ${top_srcdir}/config/startup ${PREFIX}/etc/init.d/glite-jp-primary
	${INSTALL} -m 755 ${top_srcdir}/config/glite-jp-primary-dbsetup.sql ${PREFIX}/etc
	${INSTALL} -m 755 ${top_srcdir}/config/glite-jp-primary-dbsetup.sh ${PREFIX}/etc
	${INSTALL} -m 644 ${top_srcdir}/examples/sample_job  ${PREFIX}/examples/glite-jp-primary-sample_job.lb

clean:

simple_server.o soap_ops.o jpps-test.o: ${ps_prefix}H.h

# we have no real config.h but have to force gSoap not to use
# linux ftime with broken (aka obsolete) DST information

# stdsoap2.o: ${gsoap_prefix}/devel/stdsoap2.c
# 	test -f config.h || touch config.h
# 	@echo 'The following warning "time_t (de)serialization is not MT safe on this platform" is harmless'
# 	${CC} -o $@ -c -DWITH_NONAMESPACES -DHAVE_CONFIG_H ${CFLAGS} ${gsoap_prefix}/devel/stdsoap2.c
# 

glite-jp-tags.la: tags_plugin.lo
	${SOLINK} -o $@ tags_plugin.lo

glite-jp-ftpdauth.la: ftpd_auth.lo mysql.lo
	${SOLINK} -o $@ ftpd_auth.lo mysql.lo ${COMMONLIB} ${TRIOLIB} ${MYSQLIB}

%.lo: %.c
	${LTCOMPILE} -o $@ -c $<

soap_ops.o bones_server.o: soap_version.h

soap_version.h:
	${gsoap_bin_prefix}/soapcpp2 /dev/null
	perl -ne '$$. == 2 && /.*([0-9])\.([0-9])\.([0-9]).*/ && printf "#define GSOAP_VERSION %d%02d%02d\n",$$1,$$2,$$3' soapH.h >$@
	-rm soapC.cpp soapH.h soapStub.h soapClient.cpp soapServer.cpp soapClientLib.cpp soapServerLib.cpp 
