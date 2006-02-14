# $Id$
#
# Makefile for doc

RD2		= /usr/local/bin/rd2

HTMLDIR	= html

HTMLS	= $(patsubst %.rd, $(HTMLDIR)/%.html, $(wildcard [A-Z]*.rd))
HEAD	= head.rd
IMGS	= $(patsubst images/%.png, $(HTMLDIR)/images/%.png, $(wildcard images/*.png))

all: $(HTMLS) $(IMGS)

clean:
	rm -rf $(HTMLDIR)

$(HTMLDIR)/%.html: %.rd
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cat $(HEAD) $< | ruby -Ks $(RD2) -r rd/rd2html-ext-lib.rb --out-code euc --ref-extension --headline-title --head-element --with-part=head > $@

$(HTMLDIR)/images/%.png: images/%.png
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cp $< $@
