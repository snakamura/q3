# $Id$
#
# Makefile for doc

UNAME	= $(shell uname | sed -e 's/_.*//')
RD2     = rd2
ifeq ($(UNAME),CYGWIN)
	RD2	= /usr/bin/rd2
endif
ifeq ($(UNAME),MINGW32)
	RD2	= "C:/Program Files (x86)/ruby-1.8/bin/rd2"
endif

HTMLDIR	= html
DISTDIR	= ../dist

HTMLS	= $(patsubst %.rd, $(HTMLDIR)/%.html, $(wildcard [A-Z]*.rd))
HEAD	= head.rd
FOOT	= foot.rd
IMGS	= $(patsubst images/%.png, $(HTMLDIR)/images/%.png, $(wildcard images/*.png))
CSSS	= $(patsubst %.css, $(HTMLDIR)/%.css, $(wildcard *.css))
HT		= $(HTMLDIR)/.htaccess

all: $(HTMLS) $(IMGS) $(CSSS) $(HT)

zip: all
	if [ ! -d $(DISTDIR) ]; then mkdir -p $(DISTDIR); fi
	(cd $(HTMLDIR); zip -r ../$(DISTDIR)/doc.zip *)

clean:
	rm -rf $(HTMLDIR)

$(HTMLDIR)/%.html: %.rd
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cat $(HEAD) $< $(FOOT) | $(RD2) -r rd/rd2html-ext-lib.rb --ref-extension --headline-title --head-element --with-part=head --with-part=html:include > $@

$(HTMLDIR)/%.css: %.css
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cp $< $@

$(HTMLDIR)/images/%.png: images/%.png
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cp $< $@

$(HTMLDIR)/.htaccess: .htaccess
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cp $< $@
