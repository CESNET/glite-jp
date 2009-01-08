# Default values
top_srcdir=..
builddir=build
top_builddir=${top_srcdir}/${builddir}
stagedir=.
globalprefix=glite
jpprefix=jp
package=glite-jp-ws-interface
version=0.0.0
PREFIX=/opt/glite

-include Makefile.inc

VPATH=${top_srcdir}/interface:${top_srcdir}/src
STAGETO=interface

XSLTPROC=xsltproc --nonet
XMLLINT:=xmllint --nonet
TIDY:=tidy -i -q --show-warnings no --tidy-mark no --wrap 0
docbookxls:=http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl

WSDL=JobProvenancePS.wsdl JobProvenanceIS.wsdl JobProvenanceTypes.wsdl
WSDL_S=jpdev.wsdl
XSD=JobProvenanceTypes.xsd

all compile: ${WSDL} ${XSD} JobProvenance.html ${WSDL_S}

check: 
	@echo No unit test required for interface-only module.

stage: ${WSDL}
	$(MAKE) install PREFIX=${stagedir} DOSTAGE=yes

install:
	-mkdir -p ${PREFIX}/${STAGETO}
	-mkdir -p ${PREFIX}/share/doc/${package}-${version}
	-mkdir -p ${PREFIX}/include/${globalprefix}/${jpprefix}
	install -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	-install -m 644 JobProvenance.html ${PREFIX}/share/doc/${package}-${version}
#	cd ${top_srcdir}/interface && install -m 644 ${WSDL} ${PREFIX}/${STAGETO}
	install -m 644 ${WSDL} ${WSDL_S} ${XSD} ${PREFIX}/${STAGETO}
	install -m 644 ${top_srcdir}/src/ws_fault.c ${PREFIX}/include/${globalprefix}/${jpprefix}

clean:
	rm -f *.h
	
%.wsdl: %.xml puke-wsdl.xsl
	${XSLTPROC} ../src/puke-wsdl.xsl $< >$@
	-${TIDY} -xml -m $@

%.xsd: %.xml puke-schema.xsl
	${XSLTPROC} ../src/puke-schema.xsl $< >$@
	-${TIDY} -xml -m $@

JobProvenance.html: doc.xml JobProvenancePS.xml JobProvenanceIS.xml JobProvenanceTypes.xml puke-ug.xsl
	-${XSLTPROC} --novalid ../src/puke-ug.xsl $< >doc-html.xml && \
	${XSLTPROC} --stringparam  chapter.autolabel 0 ${docbookxls} doc-html.xml >$@
	-${TIDY} -xml -m doc-html.xml
	-${XMLLINT} --valid --noout doc-html.xml
	-${TIDY} -asxhtml -m $@

jpdev.xml: jpdev.xml.sh JobProvenancePS.xml JobProvenanceIS.xml JobProvenanceTypes.xml
	${top_srcdir}/src/jpdev.sh $< > $@
