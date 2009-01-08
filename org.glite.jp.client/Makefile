# defaults
top_srcdir=..
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
gsoap_prefix=/software/gsoap-2.6

CC=gcc

-include Makefile.inc

STAGETO=include/${globalprefix}/${jpprefix}

VPATH=${top_srcdir}/src:${top_srcdir}/examples:${top_srcdir}/project$:${top_srcdir}/interface:${stagedir}/interface:${top_srcdir}/build

ifdef JP_PERF
	JP_PERF_CFLAGS:=-DJP_PERF=1
endif

GLOBUS_LIBS:=-L${globus_prefix}/lib \
	-lglobus_ftp_client_${nothrflavour} \
	-lglobus_ftp_control_${nothrflavour}

GLOBUS_CFLAGS:=-I${globus_prefix}/include/${nothrflavour}

DEBUG:=-g -O0 -DDEBUG

CFLAGS:=${DEBUG} -I. -I${top_srcdir}/interface -I${top_srcdir}/src -I${gsoap_prefix}/include -I${gsoap_prefix} -I${stagedir}/include ${GLOBUS_CFLAGS} -I${libtar}/include ${JP_PERF_CFLAGS} -W -Wall -Wno-unused-parameter -D_GNU_SOURCE
LDFLAGS:=-L${stagedir}/lib -L${libtar}/lib

LINK:=libtool --mode=link ${CC} ${LDFLAGS} 
LTCOMPILE:=libtool --mode=compile ${CC} ${CFLAGS}
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install

LIBTAR:=-L${libtar}/lib -ltar

STAGE_HDRS:=jpcl_ctx_int.h
HDRS:=jp_client.h jpimporter.h

EXAMPLES:=jpps_upload_files mill_feed

LIBOBJS:=jpcl_ctx.o jpimp_lib.o
LIBTHROBJS:=${LIBOBJS:.o=.thr.o}
LIBLOBJS:=${LIBOBJS:.o=.lo}

LIB:=libglite_jp_importer_${nothrflavour}.la
THRLIB:=libglite_jp_importer_${thrflavour}.la

daemon:=glite-jp-importer

wsprefix:=jpps_

SRCS:=jpimporter.c ${wsprefix}ClientLib.c ${wsprefix}C.c
OBJS:=${SRCS:.c=.o}

gsoap_bin_prefix:=${shell if [ -x  ${gsoap_prefix}/bin/soapcpp2 ]; then echo ${gsoap_prefix}/bin; else echo ${gsoap_prefix}; fi }
dotless_gsoap_ver:=${shell echo ${gsoap_default_version} | tr -d . }
ifeq ($(shell test -f ${stagedir}/lib/libglite_security_gsoap_plugin_${dotless_gsoap_ver}_${nothrflavour}_c.la && echo ok),ok)
       langflavour:=_c
endif
# static linking for LB/JP compatibility from different glite releases
GSOAPLIB:=-lglite_security_gsoap_plugin_${dotless_gsoap_ver}_${nothrflavour}${langflavour} -static

LBMAILDIRLIB:=-lglite_lbu_maildir

offset=0
version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }


default all: compile

compile: ${daemon} ${LIB} ${EXAMPLES}

examples: ${EXAMPLES}

${LIB}: ${LIBOBJS}
	${LINK} ${version_info} -o $@ ${LIBLOBJS} -rpath ${glite_location}/lib ${LBMAILDIRLIB} ${LIBTAR}

${daemon}: ${OBJS}
	${LINK} -o $@ ${OBJS} ${LBMAILDIRLIB} ${GSOAPLIB} ${GLOBUS_LIBS}

${EXAMPLES}: ${LIB}

jpps_upload_files: %: %.o
	${LINK} -o $@ $< ${LIB} ${LBMAILDIRLIB} 

mill_feed: %: %.o
	${LINK} -o $@ $< ${LBMAILDIRLIB} -lglite_jobid



JobProvenancePS.xh: %.xh: %.wsdl JobProvenanceTypes.wsdl typemap.dat
	cp  ${stagedir}/interface/JobProvenanceTypes.wsdl .
	${gsoap_bin_prefix}/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o $@ $<
	rm -f JobProvenanceTypes.wsdl

${wsprefix}Client.c ${wsprefix}ClientLib.c  \
${wsprefix}C.c ${wsprefix}H.h: JobProvenancePS.xh
	${gsoap_bin_prefix}/soapcpp2 -n -w -c -p ${wsprefix} JobProvenancePS.xh

env_C.c env_Server.c:
	touch env.xh
	cp  ${jpproject}/JobProvenanceTypes.wsdl .
	${gsoap_bin_prefix}/wsdl2h  -t ${top_srcdir}/src/typemap.dat -c -o env.xh JobProvenanceTypes.wsdl
	rm -f JobProvenanceTypes.wsdl
	${gsoap_bin_prefix}/soapcpp2 -w -c -p env_ env.xh

${OBJS}: ${wsprefix}H.h soap_version.h

soap_version.h:
	${gsoap_bin_prefix}/soapcpp2 /dev/null
	perl -ne '$$. == 2 && /.*([0-9]+)\.([0-9]+)\.([0-9]+)([a-z]).*/ && printf "#define GSOAP_VERSION %d%02d%02d\n#define GSOAP_VERSION_LETTER '\''$$4'\''\n",$$1,$$2,$$3' soapH.h >$@
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
	-mkdir -p ${PREFIX}/${STAGETO}
	-mkdir -p ${PREFIX}/bin 
	-mkdir -p ${PREFIX}/lib
	-mkdir -p ${PREFIX}/examples
	-mkdir -p ${PREFIX}/etc/init.d
	${INSTALL} -m 755 ${daemon} ${PREFIX}/bin
	${INSTALL} -m 644 ${LIB} ${PREFIX}/lib
	${INSTALL} -m 644 jpps_upload_files ${PREFIX}/examples/glite-jp-primary-upload_files
	${INSTALL} -m 755 mill_feed ${PREFIX}/examples/glite-jp-mill_feed
	cd ${top_srcdir}/examples && ${INSTALL} -m 755 glite-jp-importer.sh ${PREFIX}/examples/
	${INSTALL} -m 755 ${top_srcdir}/config/startup ${PREFIX}/etc/init.d/glite-jp-importer
	cd ${top_srcdir}/interface && ${INSTALL} -m 644 ${HDRS} ${PREFIX}/${STAGETO}
	if [ x${DOSTAGE} = xyes ]; then \
		cd ${top_srcdir}/interface && ${INSTALL} -m 644 ${STAGE_HDRS} ${PREFIX}/${STAGETO} ; \
	fi


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
