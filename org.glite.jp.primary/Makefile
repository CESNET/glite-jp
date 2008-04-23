# defaults
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
globalprefix=glite
jpprefix=jp
package=glite-jp-primary
version=0.0.0
PREFIX=/opt/glite

glite_location=/opt/glite
nothrflavour=gcc32
thrflavour=gcc32pthr
gsoap_prefix=/software/gsoap-2.6

CC=gcc

-include Makefile.inc


VPATH=${top_srcdir}/src:${top_srcdir}/examples:${top_srcdir}/test:${top_srcdir}/project:${stagedir}/interface

archlib:=lib
host_cpu:=${shell uname -m}
ifeq (${host_cpu},x86_64) 
    archlib:=lib64
endif   

archlib:=lib
host_cpu:=${shell uname -m}
ifeq (${host_cpu},x86_64) 
    archlib:=lib64
endif   


ifneq (${classads_prefix},/usr)
        classadslib := -L${classads_prefix}/${archlib} -lclassad
endif

CLASSADPLUGIN_LIBS:= ${classadslib} -lstdc++ 

CLASSADPLUGIN_LOBJS:= classad_plugin.lo

GLOBUS_CFLAGS:=-I${globus_prefix}/include/${nothrflavour}

DEBUG:=-g -O0  -DDEBUG

CFLAGS:=${DEBUG} -I. -I${top_srcdir}/src -I${gsoap_prefix}/include -I${gsoap_prefix} -I${stagedir}/include -I${classads_prefix}/include -I${libtar_prefix}/include ${GLOBUS_CFLAGS}
# 3.1 CFLAGS:=${DEBUG} -I. -I${top_srcdir}/interface -I${top_srcdir}/src -I${gsoap_prefix}/include -I${gsoap_prefix} -I${stagedir}/include ${GLOBUS_CFLAGS} -I${mysql_prefix}/include -I${mysql_prefix}/include/mysql -I${classads_prefix}/include -I${libtar_prefix}/include

LDFLAGS:=-L${stagedir}/lib

LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LTCOMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
SOLINK:=libtool --mode=link ${CC} -module ${LDFLAGS} -rpath ${stagedir}/lib
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

daemon:=glite-jp-primarystoraged
example:=jpps-test dag-deps
ps_prefix:=jpps_
is_prefix:=jpis_
sample_jobs:=sample_job_aborted  sample_job_cleared  sample_job_tagged_done  sample_job_waiting

plugins:=glite-jp-ftpdauth.la glite-jp-classad.la glite-jp-sandbox.la glite-jp-callouts_${nothrflavour}.la

SRCS:= bones_server.c soap_ops.c \
	new_ftp_backend.c file_plugin.c \
	feed.c authz.c attrs.c \
	tags.c\
	is_client.c \
	soap_switch.c

#	${ps_prefix}ServerLib.c \
#	${is_prefix}ClientLib.c jpps_C.c

TEST_SRCS:=jpps-test.c ${ps_prefix}C.c ${ps_prefix}Client.c
DAG_SRCS:=dag-deps.c ${ps_prefix}C.c ${ps_prefix}Client.c

gsoap_bin_prefix:=${shell if [ -x  ${gsoap_prefix}/bin/soapcpp2 ]; then echo ${gsoap_prefix}/bin; else echo ${gsoap_prefix}; fi }

OBJS:=${SRCS:.c=.o}
TEST_OBJS:=${TEST_SRCS:.c=.o}
DAG_OBJS:=${DAG_SRCS:.c=.o}

dotless_soap_ver:=${shell echo ${gsoap_version} | tr -d . }
COMMONLIB:=-lglite_jp_common_${nothrflavour}
SRVCOMMONLIB:=-lglite_jp_server_common
BONESLIB:=-lglite_lbu_server_bones
GSOAPLIB:=-L${stagedir}/lib -lglite_security_gsoap_plugin_${dotless_soap_ver}_${nothrflavour} 
TRIOLIB:=-lglite_lbu_trio
LIBTARLIB:=-L${libtar_prefix}/lib -ltar
GSSLIB:=-lglite_security_gss_${nothrflavour}

default all: compile

compile: ${daemon} ${example} ${plugins}

${daemon}: ${OBJS}
	${LINK} -o $@ -export-dynamic ${OBJS} ${BONESLIB} ${TRIOLIB} ${COMMONLIB} ${GSOAPLIB} ${GSSLIB} ${SRVCOMMONLIB}

jpps-test: ${TEST_OBJS}
	${LINK} -o $@ ${TEST_OBJS} ${GSOAPLIB}

dag-deps: ${DAG_OBJS}
	${LINKXX} -o $@ ${DAG_OBJS} ${classadslib} ${GSOAPLIB}

JobProvenancePS.xh: %.xh: %.wsdl JobProvenanceTypes.wsdl typemap.dat
	cp  ${stagedir}/interface/JobProvenanceTypes.wsdl .
	${gsoap_bin_prefix}/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o $@ $<
	rm -f JobProvenanceTypes.wsdl

JobProvenanceIS.xh: %.xh: %.wsdl JobProvenanceTypes.wsdl typemap.dat
	cp  ${stagedir}/interface/JobProvenanceTypes.wsdl .
	${gsoap_bin_prefix}/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o $@ $<
	rm -f JobProvenanceTypes.wsdl

glite-jp-classad.la: ${CLASSADPLUGIN_LOBJS}
	${SOLINK} -o $@ ${CLASSADPLUGIN_LOBJS} ${CLASSADPLUGIN_LIBS}

${ps_prefix}Client.c ${ps_prefix}ClientLib.c  \
${ps_prefix}Server.c ${ps_prefix}ServerLib.c  \
${ps_prefix}C.c ${ps_prefix}H.h: JobProvenancePS.xh
	${gsoap_bin_prefix}/soapcpp2 -n -w -c -p ${ps_prefix} JobProvenancePS.xh

${is_prefix}ClientLib.c ${is_prefix}Client.c \
${is_prefix}C.c ${is_prefix}H.h: JobProvenanceIS.xh
	${gsoap_bin_prefix}/soapcpp2 -n -w -c -p ${is_prefix} JobProvenanceIS.xh

soap_ops.o: ${ps_prefix}ServerLib.c soap_util.c soap_env_ctx.c soap_env_ctx.h

is_client.o: ${is_prefix}ClientLib.c soap_util.c soap_env_ctx.c soap_env_ctx.h

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

install:
	-mkdir -p ${PREFIX}/bin ${PREFIX}/etc ${PREFIX}/examples ${PREFIX}/etc/init.d  ${PREFIX}/lib
	${INSTALL} -m 755 ${daemon} ${PREFIX}/bin
	${INSTALL} -m 755 jpps-test ${PREFIX}/examples/glite-jp-primary-test
	${INSTALL} -m 755 ${top_srcdir}/examples/getjobattr.pl ${PREFIX}/examples/glite-jpps-getjobattr.pl
	for plugin in ${plugins}; do \
		${INSTALL} -m 755 $$plugin ${PREFIX}/lib; \
	done
	${INSTALL} -m 755 ${top_srcdir}/config/startup ${PREFIX}/etc/init.d/glite-jp-primary
	${INSTALL} -m 755 ${top_srcdir}/config/glite-jp-primary-dbsetup.sql ${PREFIX}/etc
	${INSTALL} -m 755 ${top_srcdir}/config/glite-jp-primary-dbsetup.sh ${PREFIX}/etc	
	${INSTALL} -m 755 ${top_srcdir}/examples/jpps_store_test ${PREFIX}/examples/glite-jp-primary-store-test
	for i in ${sample_jobs}; do \
		${INSTALL} -m 644 ${top_srcdir}/examples/$$i  ${PREFIX}/examples/glite-jp-primary-$$i.lb; \
	done
	-mkdir -p ${PREFIX}/yaim/functions/
	-mkdir -p ${PREFIX}/yaim/node-info.d
	-mkdir -p ${PREFIX}/yaim/defaults
	${INSTALL} -m 0644 ${top_srcdir}/config/functions/config* ${PREFIX}/yaim/functions
	${INSTALL} -m 0644 ${top_srcdir}/config/node-info.d/glite* ${PREFIX}/yaim/node-info.d
	${INSTALL} -m 0644 ${top_srcdir}/config/defaults/glite* ${PREFIX}/yaim/defaults
clean:

simple_server.o soap_ops.o jpps-test.o: ${ps_prefix}H.h

# we have no real config.h but have to force gSoap not to use
# linux ftime with broken (aka obsolete) DST information

# stdsoap2.o: ${gsoap_prefix}/devel/stdsoap2.c
# 	test -f config.h || touch config.h
# 	@echo 'The following warning "time_t (de)serialization is not MT safe on this platform" is harmless'
# 	${CC} -o $@ -c -DWITH_NONAMESPACES -DHAVE_CONFIG_H ${CFLAGS} ${gsoap_prefix}/devel/stdsoap2.c
# 

glite-jp-sandbox.la: sandbox_plugin.lo
	${SOLINK} -o $@ sandbox_plugin.lo ${LIBTARLIB}

glite-jp-ftpdauth.la: ftpd_auth.lo
	${SOLINK} -o $@ ftpd_auth.lo ${COMMONLIB} ${TRIOLIB} ${SRVCOMMONLIB}

glite-jp-callouts_${nothrflavour}.la: jp_callouts.lo
	${SOLINK} -o $@ $^ ${COMMONLIB} ${TRIOLIB} ${SRVCOMMONLIB}

#glite-jp-classad.lo: classad_plugin.c
#	${LTCOMPILE} -DPLUGIN_DEBUG -o $@ -c $<

#glite-jp-classad.lo: classad_plugin.c
#	${LTCOMPILE} -DPLUGIN_DEBUG -o $@ -c $<

%.lo: %.c
	${LTCOMPILE} -o $@ -c $<

soap_ops.o bones_server.o: soap_version.h

soap_version.h:
	${gsoap_bin_prefix}/soapcpp2 /dev/null
	perl -ne '$$. == 2 && /.*([0-9])\.([0-9])\.([0-9]).*/ && printf "#define GSOAP_VERSION %d%02d%02d\n",$$1,$$2,$$3' soapH.h >$@
	-rm soapC.cpp soapH.h soapStub.h soapClient.cpp soapServer.cpp soapClientLib.cpp soapServerLib.cpp 

soap_env_ctx.c soap_env_ctx.h soap_switch.c: env_C.c mk_soap_switch.pl
	${top_srcdir}/src/mk_soap_switch.pl env_C.c >soap_switch.c
