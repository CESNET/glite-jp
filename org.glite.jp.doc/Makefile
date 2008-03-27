# Default values
top_srcdir=.
stagedir=.
globalprefix=glite
lbprefix=lb
package=glite-jp-doc
version=0.0.0
PREFIX=/opt/glite

-include Makefile.inc

VPATH = ${top_srcdir}/src
KPATH = TEXINPUTS=".:$(VPATH)//:"
KPATHBIB = BIBINPUTS=".:$(VPATH)//:"

LATEX = $(KPATH) latex
PDFLATEX = $(KPATH) pdflatex
BIBTEX = $(KPATHBIB) bibtex
DVIPS = $(KPATH) dvips
AT3=${stagedir}/sbin/glite-lb-at3
INSTALL=install

default all: generate JPUG.pdf JPAG.pdf JPDG.pdf

generate: 

# %.dvi: %.tex
# 	$(LATEX) $<
# 	$(BIBTEX) `basename $< .tex`
# 	$(LATEX) $<
# 	$(LATEX) $<
# 
# %.ps: %.dvi
# 	$(DVIPS) -ta4 -o $@ $<

%.pdf: %.tex
	$(PDFLATEX) $<
	$(BIBTEX) `basename $< .tex`
	$(PDFLATEX) $<
	$(PDFLATEX) $<

%.tex: %.tex.T
	rm -f $@
	${AT3} $< >$@ || rm -f $@
	chmod -w $@ >/dev/null

install:
	-mkdir -p ${PREFIX}/share/doc/${package}-${version}
	$(INSTALL) -m 644 ${top_srcdir}/LICENSE ${PREFIX}/share/doc/${package}-${version}
	$(INSTALL) -m 644 ${top_srcdir}/src/README ${PREFIX}/share/doc/${package}-${version}
	$(INSTALL) -m 644 JPUG.pdf ${PREFIX}/share/doc/${package}-${version}
	$(INSTALL) -m 644 JPAG.pdf ${PREFIX}/share/doc/${package}-${version}
	$(INSTALL) -m 644 JPDG.pdf ${PREFIX}/share/doc/${package}-${version}

clean:
	rm -rvf JPUG* JPAG* JPDG* 
	rm -rvf log.xml project/ rpmbuild/ RPMS/ tgz/


# dependencies:

JPUG.pdf: JPUG.tex \
	JPUG-Introduction.tex \
	JPUG-Tools.tex glite-jpis-client.tex jpimporter.tex gui.tex \
	JPUG-UseCases.tex 

JPAG.pdf: JPAG.tex \
	JPAG-Introduction.tex LB-JP-interaction.tex \
	JPAG-Installation.tex \
	JPAG-Configuration.tex \
	JPAG-Running.tex \
	JPAG-Testing.tex \
	JPAG-Troubleshooting.tex

JPDG.pdf: JPDG.tex \
	JPDG-Introduction.tex \
	JPDG-WS.tex

.PHONY: all clean
