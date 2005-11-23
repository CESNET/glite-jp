# defaults
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
jpprefix=jp
package=glite-jp-server-common
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


VPATH=${top_srcdir}/src:${top_srcdir}/examples:${top_srcdir}/test:${top_srcdir}/project:${jpproject}:${stagedir}/interface

DEBUG:=-g -O0 -W -Wall -DDEBUG
CPPFLAGS:=-I. -I${top_srcdir}/interface -I${top_srcdir}/src -I${stagedir}/include -I${mysql_prefix}/include -I${mysql_prefix}/include/mysql
CFLAGS:=${DEBUG} -D_GNU_SOURCE
LDFLAGS:=-L${stagedir}/lib

offset=0
version_info:=-version-info ${shell \
	perl -e '$$,=":"; @F=split "\\.","${version}"; print $$F[0]+$$F[1]+${offset},$$F[2],$$F[1]' }


COMPILE:=libtool --mode=compile ${CC} ${CPPFLAGS} ${CFLAGS}
LINK:=libtool --mode=link ${CC} -rpath ${stagedir}/lib ${LDFLAGS} 
LIBLINK:=${LINK} ${version_info}
INSTALL:=libtool --mode=install install

GLOBUS_LIBS:=-L${globus_prefix}/lib \
	-lglobus_common_${nothrflavour} \
	-lglobus_gssapi_gsi_${nothrflavour}

TRIOLIB:=-lglite_lb_trio

STATICLIB:=libglite_jp_server_common.a
LTLIB:=libglite_jp_server_common.la


SRCS:=db.c
HDRS:=db.h
OBJS:=${SRCS:.c=.o}
LOBJS:=${OBJS:.o=.lo}

ifneq (${mysql_prefix},/usr)
	ifeq ($(shell echo ${mysql_version} | cut -d. -f1,2),4.1)
		MYSQLIB := -L${mysql_prefix}/lib/mysql -lmysqlclient
	else
		MYSQLIB := -L${mysql_prefix}/lib -lmysqlclient
	endif
else
	MYSQLIB := -lmysqlclient
endif


default all: compile examples

compile: ${LTLIB} ${STATICLIB}

${LTLIB} ${STATICLIB}: ${OBJS}
	${LIBLINK} ${version_info} -o $@ ${LOBJS} ${MYSQLIB}

examples: db-test-int

db-test-int: db-test-int.o
	${LINK} -o $@ $+ ${STATICLIB} ${MYSQLIB} -lglite_jp_common ${TRIOLIB} ${GLOBUS_LIBS}

check: 
	-echo nothing yet

doc:

stage: compile
	$(MAKE) install PREFIX=${stagedir}

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
	-mkdir -p  ${PREFIX}/lib ${PREFIX}/include/${globalprefix}/${jpprefix}
	${INSTALL} -m 755 ${LTLIB} ${PREFIX}/lib
	for f in ${HDRS}; do \
		${INSTALL} -m 644 ${top_srcdir}/interface/"$$f" ${PREFIX}/include/${globalprefix}/${jpprefix}; \
	done

clean:

%.o: %.c
	${COMPILE} -c $< -o $@

.PHONY: default all compile examples check doc stage dist distsrc distbin install clean