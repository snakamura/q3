# $Id$
#
# Makefile for doc

RD2		= /usr/local/bin/rd2

HTMLDIR	= html
DISTDIR	= ../dist

HTMLS	= $(patsubst %.rd, $(HTMLDIR)/%.html, $(wildcard [A-Z]*.rd))
HEAD	= head.rd
FOOT	= foot.rd
IMGS	= $(patsubst images/%.png, $(HTMLDIR)/images/%.png, $(wildcard images/*.png))
CSSS	= $(patsubst %.css, $(HTMLDIR)/%.css, $(wildcard *.css))

all: $(HTMLS) $(IMGS) $(CSSS)

zip: all
	if [ ! -d $(DISTDIR) ]; then mkdir -p $(DISTDIR); fi
	(cd $(HTMLDIR); zip -r ../$(DISTDIR)/doc.zip *)

clean:
	rm -rf $(HTMLDIR)

$(HTMLDIR)/%.html: %.rd
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cat $(HEAD) $< $(FOOT) | ruby -W0 -Ks $(RD2) -r rd/rd2html-ext-lib.rb --ref-extension --headline-title --head-element --with-part=head --with-part=html:include > $@

$(HTMLDIR)/%.css: %.css
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cp $< $@

$(HTMLDIR)/images/%.png: images/%.png
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cp $< $@