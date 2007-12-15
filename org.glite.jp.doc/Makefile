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

clean:
	rm -rvf JPUG* JPAG* JPDG* 

.PHONY: all clean purge ps pdf
