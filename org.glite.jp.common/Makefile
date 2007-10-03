# defaults
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
globalprefix=glite
jpprefix=jp
package=glite-jp-common
version=0.0.0
PREFIX=/opt/glite

globus_prefix=/opt/globus
nothrflavour=gcc32
thrflavour=gcc32pthr
expat_prefix=/opt/expat
gsoap_prefix=/software/gsoap-2.6

CC=gcc

-include Makefile.inc


VPATH=${top_srcdir}/src:${top_srcdir}/test:${top_srcdir}/project:${jpproject}

GLOBUSINC:= -I${globus_prefix}/include/${nothrflavour}
GLOBUSINCTHR:= -I${globus_prefix}/include/${thrflavour}

DEBUG:=-g -O0 
CFLAGS:=${DEBUG} -I. -I${top_srcdir}/interface -I${stagedir}/include

offset=0
version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }


LINK:=libtool --mode=link ${CC} ${LDFLAGS} -rpath ${stagedir}/lib ${version_info}
LINKXX:=libtool --mode=link ${CXX} ${LDFLAGS} 
INSTALL:=libtool --mode=install install
COMPILE:=libtool --mode=compile ${CC} ${CFLAGS}

HDRS:=types.h context.h strmd5.h attr.h known_attr.h

SRCS:=context.c strmd5.c attr.c
OBJS:=${SRCS:.c=.lo}
THROBJS:=${OBJS:.o=.thr.lo}
LIBS:=-L${globus_prefix}/lib -lcrypto_${nothrflavour}
THRLIBS:=-L${globus_prefix}/lib -lcrypto_${thrflavour}

commonlib:= libglite_jp_common_${nothrflavour}.la
commonlib_thr:= libglite_jp_common_${thrflavour}.la

TEST_LIBS:=-L${cppunit}/lib -lcppunit -ldl
TEST_INC:=-I${cppunit}/include


default all: compile

compile: ${commonlib} ${commonlib_thr}

${commonlib}: ${OBJS}
	${LINK} -o $@ ${OBJS} ${LIBS}

${commonlib_thr}: ${THROBJS}
	${LINK} -o $@ ${THROBJS} ${THRLIBS}

check: base64_test type_test
	./base64_test base64_test.xml
	./type_test type_test.xml

type_test base64_test: %: %.cpp compile
	${CXX} -c ${CFLAGS} ${TEST_INC} $<
	${LINKXX} -o $@ $@.o ${commonlib} ${TEST_LIBS} 

doc:

stage: compile
	$(MAKE) install PREFIX=${stagedir}

install:
	-mkdir -p ${PREFIX}/include/${globalprefix}/${jpprefix}
	cd ${top_srcdir}/interface && install -m 644 ${HDRS} ${PREFIX}/include/${globalprefix}/${jpprefix}
	-mkdir -p ${PREFIX}/lib
	${INSTALL} -m 755 ${commonlib} ${commonlib_thr} ${PREFIX}/lib

clean:
	rm -rvf *.o *.lo .libs lib*
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/

%.thr.lo: %.c
	${COMPILE} ${GLOBUSTHRINC} -o $@ -c $<

%.lo: %.c
	${COMPILE} ${GLOBUSINC} -o $@ -c $< 
