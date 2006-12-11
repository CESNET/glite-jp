# Default values
top_srcdir=.
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
distdir=.
globalprefix=glite
package=glite-jp-ws-interface
version=0.0.0
PREFIX=/opt/glite

-include Makefile.inc

VPATH=${top_srcdir}/interface:${top_srcdir}/src
STAGETO=interface

XSLTPROC=xsltproc --nonet
XMLLINT:=xmllint --nonet
TIDY:=tidy
docbookxls:=http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl

WSDL=JobProvenancePS.wsdl JobProvenanceIS.wsdl JobProvenanceTypes.wsdl 
XSD=JobProvenanceTypes.xsd

all compile: ${WSDL} ${XSD} JobProvenance.html

check: 
	@echo No unit test required for interface-only module.

stage: ${WSDL}
	$(MAKE) install PREFIX=${stagedir}

dist: distsrc distbin

distsrc:
	mkdir -p ${top_srcdir}/${package}-${version}
	cd ${top_srcdir} && GLOBIGNORE="${package}-${version}" && cp -Rf * ${package}-${version}
	cd ${top_srcdir} && tar -czf ${distdir}/${package}-${version}_src.tar.gz --exclude-from=project/tar_exclude ${package}-${version}
	rm -rf ${top_srcdir}/${package}-${version}

distbin:
	$(MAKE) install PREFIX=${top_srcdir}/tmpbuilddir
	cd ${top_srcdir}/tmpbuilddir && tar -czf ${top_srcdir}/${distdir}/${package}-${version}_bin.tar.gz *
	rm -rf ${top_srcdir}/tmpbuilddir
	
install:
	-mkdir -p ${PREFIX}/${STAGETO}
	-mkdir -p ${PREFIX}/share/doc/${package}-${version}
	install -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	install -m 644 JobProvenance.html ${PREFIX}/share/doc/${package}-${version}
#	cd ${top_srcdir}/interface && install -m 644 ${WSDL} ${PREFIX}/${STAGETO}
	install -m 644 ${WSDL} ${XSD} ${PREFIX}/${STAGETO}

clean:
	rm -f *.h
	
%.wsdl: %.xml puke-wsdl.xsl
	${XSLTPROC} ../src/puke-wsdl.xsl $< >$@
	-${TIDY} -wrap 10000 -xml -m -i -q $@

%.xsd: %.xml puke-schema.xsl
	${XSLTPROC} ../src/puke-schema.xsl $< >$@
	-${TIDY} -wrap 10000 -xml -m -i -q $@

JobProvenance.html: doc.xml JobProvenancePS.xml JobProvenanceIS.xml JobProvenanceTypes.xml puke-ug.xsl
	${XSLTPROC} --novalid ../src/puke-ug.xsl $< >doc-html.xml
	-${TIDY} -wrap 10000 -xml -m -i -q doc-html.xml
	${XMLLINT} --valid --noout doc-html.xml
	${XSLTPROC} --stringparam  chapter.autolabel 0 ${docbookxls} doc-html.xml >$@
	-${TIDY} -wrap 10000 -asxhtml -m -i -q $@
