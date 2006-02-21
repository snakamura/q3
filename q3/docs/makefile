# $Id$
#
# Makefile for doc

RD2		= /usr/local/bin/rd2

HTMLDIR	= html
ZIPDIR	= ../zip

HTMLS	= $(patsubst %.rd, $(HTMLDIR)/%.html, $(wildcard [A-Z]*.rd))
HEAD	= head.rd
FOOT	= foot.rd
IMGS	= $(patsubst images/%.png, $(HTMLDIR)/images/%.png, $(wildcard images/*.png))

all: $(HTMLS) $(IMGS)

zip: all
	if [ ! -d $(ZIPDIR) ]; then mkdir -p $(ZIPDIR); fi
	(cd $(HTMLDIR); zip -r ../$(ZIPDIR)/doc.zip *)

clean:
	rm -rf $(HTMLDIR)

$(HTMLDIR)/%.html: %.rd
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cat $(HEAD) $< $(FOOT) | ruby -Ks $(RD2) -r rd/rd2html-ext-lib.rb --out-code sjis --ref-extension --headline-title --head-element --with-part=head --with-part=html:include > $@

$(HTMLDIR)/images/%.png: images/%.png
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cp $< $@
