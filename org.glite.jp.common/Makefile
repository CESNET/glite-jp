# defaults
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
lbprefix=lb
package=glite-jp-common
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


VPATH=${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/project:${jpproject}

GLOBUSINC:= -I${globus_prefix}/include/${nothrflavour}


DEBUG:=-g -O0 
CFLAGS:=${DEBUG} -I. -I${top_srcdir}/interface -I${stagedir}/include \
	${GLOBUSINC}

LINK:=libtool --mode=link ${CC} ${LDFLAGS} -rpath ${stagedir}/lib
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install
COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}

HDRS:=types.h context.h strmd5.h attr.h

SRCS:=context.c strmd5.c attr.c
OBJS:=${SRCS:.c=.lo}

commonlib:= libglite_jp_common.la

default all: compile

compile: ${commonlib}

${commonlib}: ${OBJS}
	${LINK} -o $@ ${OBJS}

check: 
	-echo nothing yet

doc:

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install:
	-mkdir -p ${PREFIX}/include/${globalprefix}/${jpprefix}
	cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${PREFIX}/include/${globalprefix}/${jpprefix}
	-mkdir -p ${PREFIX}/lib
	${INSTALL} -m 755 ${commonlib} ${PREFIX}/lib

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
        
clean:

%.lo: %.c
	${COMPILE} -o $@ -c $< 
