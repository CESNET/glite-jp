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
gsoap_prefix=/software/gsoap-2.6

CC=gcc

-include Makefile.inc


VPATH=${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/project:${jpproject}

GLOBUSINC:= -I${globus_prefix}/include/${nothrflavour}


DEBUG:=-g -O0 
CFLAGS:=${DEBUG} -I. -I${top_srcdir}/interface -I${stagedir}/include \
	${GLOBUSINC}

LDFLAGS:=-L${globus_prefix}/lib

offset=0
version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }


LINK:=libtool --mode=link ${CC} ${LDFLAGS} -rpath ${stagedir}/lib ${version_info}
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install
COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}

HDRS:=types.h context.h strmd5.h attr.h known_attr.h trio.h escape.h

SRCS:=context.c strmd5.c attr.c
OBJS:=${SRCS:.c=.lo}
LIBS:=-lcrypto_${nothrflavour}
LIBS_THR:=-lcrypto_${thrflavour}

commonlib:= libglite_jp_common_${nothrflavour}.la
commonlib_thr:= libglite_jp_common_${thrflavour}.la

TRIO_OBJS:=escape.o trio.o strio.o
TRIO_LIB:=libglite_jp_trio.la
TRIO_LOBJS:=${TRIO_OBJS:.o=.lo}

TEST_LIBS:=-L${cppunit}/lib -lcppunit -ldl
TEST_INC:=-I${cppunit}/include


default all: compile

compile: ${commonlib} ${commonlib_thr} ${TRIO_LIB} 

${commonlib}: ${OBJS}
	${LINK} -o $@ ${OBJS} ${LIBS}

${commonlib_thr}: ${OBJS}
	${LINK} -o $@ ${OBJS} ${LIBS}

${TRIO_LIB}: ${TRIO_LOBJS}
	${LINK} ${version_info} -o $@ ${TRIO_LOBJS} -lm

check: base64_test type_test
	./base64_test base64_test.xml
	./type_test type_test.xml

type_test base64_test: %: %.cpp compile
	${CXX} -c ${CFLAGS} ${TEST_INC} $<
	${LINKXX} -o $@ $@.o ${commonlib} ${TRIO_LIB} ${TEST_LIBS} 

doc:

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install:
	-mkdir -p ${PREFIX}/include/${globalprefix}/${jpprefix}
	cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${PREFIX}/include/${globalprefix}/${jpprefix}
	-mkdir -p ${PREFIX}/lib
	${INSTALL} -m 755 ${commonlib} ${commonlib_thr} ${TRIO_LIB} ${PREFIX}/lib

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

${TRIO_LOBJS}: %.lo: %.c
	${COMPILE} -o $@ -c -DDATAGRID_EXTENSION $< 
