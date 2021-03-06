# defaults
top_srcdir=..
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
nothrflavour=gcc32
thrflavour=gcc32pthr

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

STATICLIB:=libglite_jp_server_common.a
LTLIB:=libglite_jp_server_common.la


SRCS:=db.c
HDRS:=db.h
OBJS:=${SRCS:.c=.o}
LOBJS:=${OBJS:.o=.lo}

default all: compile examples

compile: ${LTLIB} ${STATICLIB}

${LTLIB} ${STATICLIB}: ${OBJS}
	${LIBLINK} -o $@ ${LOBJS} -lglite_lbu_db 

examples:

check: 
	-echo nothing yet

doc:

stage: compile
	$(MAKE) install PREFIX=${stagedir}

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
