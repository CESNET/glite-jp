# defaults
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
jpprefix=jp
package=glite-jp-client
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


VPATH=${top_srcdir}/src:${top_srcdir}/examples:${top_srcdir}/project:${stagedir}/interface

GLOBUS_LIBS:=-L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} \
	-lglobus_gssapi_gsi_${nothrflavour} \
	-lglobus_ftp_client_gcc32dbg \
	-lglobus_ftp_control_gcc32dbg

GLOBUS_CFLAGS:=-I${globus_prefix}/include/${nothrflavour}

DEBUG:=-g -O0 -DDEBUG

CFLAGS:=${DEBUG} -I. -I${top_srcdir}/interface -I${top_srcdir}/src -I${gsoap_prefix}/include -I${stagedir}/include ${GLOBUS_CFLAGS}
LDFLAGS:=-L${stagedir}/lib

LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LTCOMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

daemon:=glite-jp-importer

wsprefix:=jpps_

SRCS:= jpimporter.c \
	${wsprefix}ClientLib.c ${wsprefix}C.c
#	env_C.c

EXA_SRCS:=

OBJS:=${SRCS:.c=.o}
EXA_OBJS:=${EXA_SRCS:.c=.o}

COMMONLIB:=-lglite_jp_common
GSOAPLIB:=-lglite_security_gsoap_plugin_${nothrflavour} -lglite_security_gss_${nothrflavour} \
	-L${gsoap_prefix}/lib -lgsoap${GSOAP_DEBUG} -L${ares_prefix}/lib -lares
LBMAILDIRLIB:=-lglite_lb_maildir

default all: compile

compile: ${daemon} 

${daemon}: ${OBJS}
	${LINK} -o $@ ${OBJS} ${LBMAILDIRLIB} ${COMMONLIB} ${GSOAPLIB} ${GLOBUS_LIBS}


JobProvenancePS.xh: %.xh: %.wsdl JobProvenanceTypes.wsdl typemap.dat
	cp  ${stagedir}/interface/JobProvenanceTypes.wsdl .
	${gsoap_prefix}/bin/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o $@ $<
	rm -f JobProvenanceTypes.wsdl

${wsprefix}Client.c ${wsprefix}ClientLib.c  \
${wsprefix}C.c ${wsprefix}H.h: JobProvenancePS.xh
	${gsoap_prefix}/bin/soapcpp2 -n -w -c -p ${wsprefix} JobProvenancePS.xh

env_C.c env_Server.c:
	touch env.xh
	cp  ${jpproject}/JobProvenanceTypes.wsdl .
	${gsoap_prefix}/bin/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o env.xh JobProvenanceTypes.wsdl
	rm -f JobProvenanceTypes.wsdl
	${gsoap_prefix}/bin/soapcpp2 -w -c -p env_ env.xh

${OBJS}: ${wsprefix}H.h soap_version.h

soap_version.h:
	${gsoap_prefix}/bin/soapcpp2 /dev/null
	perl -ne '$$. == 2 && /.*([0-9])\.([0-9])\.([0-9]).*/ && printf "#define GSOAP_VERSION %d%02d%02d\n",$$1,$$2,$$3' soapH.h >$@
	-rm soapC.cpp soapH.h soapStub.h soapClient.cpp soapServer.cpp soapClientLib.cpp soapServerLib.cpp




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

clean:

# we have no real config.h but have to force gSoap not to use
# linux ftime with broken (aka obsolete) DST information
stdsoap2.o: ${gsoap_prefix}/devel/stdsoap2.c
	test -f config.h || touch config.h
	@echo 'The following warning "time_t (de)serialization is not MT safe on this platform" is harmless'
	${CC} -o $@ -c -DWITH_NONAMESPACES -DHAVE_CONFIG_H ${CFLAGS} ${gsoap_prefix}/devel/stdsoap2.c


%.lo: %.c
	${LTCOMPILE} -o $@ -c $<

%.o: %.c
	${LTCOMPILE} -o $@ -c $<
