
DOCS = \
	kumo-gateway.1 \
	kumo-manager.1 \
	kumo-server.1 \
	kumoctl.1 \
	kumohash.1 \
	kumolog.1 \
	kumomergedb.1 \
	kumostat.1 \
	kumotop.1

all: $(DOCS)

clean: kumo-gateway.1.clean kumo-manager.1.clean kumo-server.1.clean kumoctl.1.clean kumohash.1.clean kumolog.1.clean kumomergedb.1.clean kumostat.1.clean kumotop.1.clean

%.sdoc : %.txt
	wfdoc -i $< -o $@

%.html : %.txt
	wfdoc -f html -i $< -o $@

%.tex : %.sdoc
	sdoc -toc -format:latex2e -latex2e.driver=dvipdfm -latex2e.ref=HyperRefLaTeX2eRefHandler $<

%.etex : %.tex
	wftex $< $@

%.toc : %.etex
	platex $<

%.dvi : %.etex %.toc
	platex $<

%.pdf : %.dvi
	dvipdfmx $<
	rm -f $*.aux $*.log

%.clean : %.txt
	rm -f $*.sdoc $*.tex $*.etex $*.dvi $*.toc $*.aux $*.out $*.log $*.pdf

%.distclean : %.txt
	rm -f $*.sdoc $*.tex $*.etex $*.dvi $*.toc $*.aux $*.out $*.log $*.pdf $*.mk

%.1 : %.1.txt
	wfdoc -f man $< -o $@

