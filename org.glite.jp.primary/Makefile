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


VPATH=${top_srcdir}/src:${top_srcdir}/examples:${top_srcdir}/test:${top_srcdir}/project:${jpproject}

GLOBUS_LIBS:=-L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} \
	-lglobus_gssapi_gsi_${nothrflavour}

DEBUG:=-g -O0  -DDEBUG

CFLAGS:=${DEBUG} -I. -I${top_srcdir}/src -I${gsoap_prefix}/include -I${stagedir}/include
LDFLAGS:=-L${stagedir}/lib

LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install


daemon:=glite-jp-primarystoraged
example:=jpps-test
ps_prefix:=jpps_
is_prefix:=jpis_

gsoap_version=`${gsoap_prefix}/bin/soapcpp2 -version 2>&1 | cut -d' ' -f4 | perl -F\\\\. -nae '$$F[2] =~ s/\D*$$//; print $$F[2]+100*$$F[1]+10000*$$F[0]'`

CFLAGS+=-DGSOAP_VERSION=${gsoap_version}


SRCS:= simple_server.c soap_ops.c \
	ftp_backend.c \
	feed.c tags.c\
	is_client.c \
	${ps_prefix}ServerLib.c \
	${is_prefix}ClientLib.c jpps_C.c
#	env_C.c

EXA_SRCS:=jpps-test.c ${ps_prefix}C.c ${ps_prefix}Client.c


OBJS:=${SRCS:.c=.o}
EXA_OBJS:=${EXA_SRCS:.c=.o}

COMMONLIB:=-lglite_jp_common
GSOAPLIB:=-L${gsoap_prefix}/lib -lgsoap${GSOAP_DEBUG}

default all: compile

compile: ${daemon} ${example}

${daemon}: ${OBJS}
	${LINK} -o $@ ${OBJS} ${COMMONLIB} ${GSOAPLIB} ${GLOBUS_LIBS} 

${example}: ${EXA_OBJS}
	${LINK} -o $@ ${EXA_OBJS} ${GSOAPLIB}  

JobProvenanceIS.xh JobProvenancePS.xh: %.xh: %.wsdl JobProvenanceTypes.wsdl typemap.dat
	cp  ${jpproject}/JobProvenanceTypes.wsdl .
	${gsoap_prefix}/bin/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o $@ $<
	rm -f JobProvenanceTypes.wsdl

${ps_prefix}Client.c ${ps_prefix}ClientLib.c  \
${ps_prefix}Server.c ${ps_prefix}ServerLib.c  \
${ps_prefix}C.c ${ps_prefix}H.h: JobProvenancePS.xh
	${gsoap_prefix}/bin/soapcpp2 -n -w -c -p ${ps_prefix} JobProvenancePS.xh

${is_prefix}ClientLib.c ${is_prefix}Client.c \
${is_prefix}C.c ${is_prefix}H.h: JobProvenanceIS.xh
	${gsoap_prefix}/bin/soapcpp2 -n -w -c -p ${is_prefix} JobProvenanceIS.xh

env_C.c env_Server.c:
	touch env.xh
	cp  ${jpproject}/JobProvenanceTypes.wsdl .
	${gsoap_prefix}/bin/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o env.xh JobProvenanceTypes.wsdl
	rm -f JobProvenanceTypes.wsdl
	${gsoap_prefix}/bin/soapcpp2 -w -c -p env_ env.xh

#$(SOAP_PREFIX)H.h $(SOAP_PREFIX)C.c: LB.xh
#        $(GSOAP_BIN_PATH)/soapcpp2 -w -c -p $(SOAP_PREFIX) LB.xh
#
#LB.xh: LB.wsdl typemap.dat
#         $(GSOAP_BIN_PATH)/wsdl2h -c -o $@ LB.wsdl
#


simple_server.o: ${is_prefix}H.h ${ps_prefix}H.h

check: 
	-echo nothing yet

doc:

stage: compile
	${INSTALL} -m 755 ${daemon} ${stagedir}/bin

dist: distsrc distbin

# FIXME: just copied from LB
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
	for p in bkserverd bkindex; do \
		${INSTALL} -m 755 "glite_lb_$$p" "${PREFIX}/bin/glite-lb-$$p"; \
	done

	for f in dbsetup.sql index.conf.template; do \
		${INSTALL} -m 644 ${top_srcdir}/config/"glite-lb-$$f" ${PREFIX}/etc; \
	done

	${INSTALL} -m 755 ${top_srcdir}/config/startup ${PREFIX}/etc/init.d/glite-lb-bkserverd

clean:

simple_server.o soap_ops.o jpps-test.o: ${ps_prefix}H.h

# we have no real config.h but have to force gSoap not to use
# linux ftime with broken (aka obsolete) DST information

stdsoap2.o: ${gsoap_prefix}/devel/stdsoap2.c
	test -f config.h || touch config.h
	@echo 'The following warning "time_t (de)serialization is not MT safe on this platform" is harmless'
	${CC} -o $@ -c -DWITH_NONAMESPACES -DHAVE_CONFIG_H ${CFLAGS} ${gsoap_prefix}/devel/stdsoap2.c

