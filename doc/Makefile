TEXMK=latex-mk
TEXMKOPT=--ignore-errors --bibtex
TEX=platex
TEXOPT=
BIBTEX=pbibtex
BIBTEXOPT=
DVIPDF=dvipdfmx
DVIPDFOPT=-d5

all: syntax.pdf

syntax.pdf: syntax.dvi
	$(DVIPDF) $(DVIPDFOPT) syntax.dvi

syntax.dvi: syntax.tex
	LATEX=$(TEX) BIBTEX=$(BIBTEX) $(TEXMK) $(TEXMKOPT) $(TEXOPT) syntax.tex

.PHONY: clean all

