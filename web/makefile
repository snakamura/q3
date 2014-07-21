# $Id: makefile 3581 2006-10-20 12:52:41Z snakamura $
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
ZIPDIR	= ../zip

HTMLS	= $(patsubst %.rd, $(HTMLDIR)/%.html, $(wildcard [a-zA-Z]*.rd */[a-zA-Z]*.rd))
HEAD	= _head.rd
FOOT	= _foot.rd
IMGS	= $(patsubst images/%.png, $(HTMLDIR)/images/%.png, $(wildcard images/*.png))
CSSS	= $(patsubst %.css, $(HTMLDIR)/%.css, $(wildcard *.css))

all: $(HTMLS) $(IMGS) $(CSSS)

zip: all
	if [ ! -d $(ZIPDIR) ]; then mkdir -p $(ZIPDIR); fi
	(cd $(HTMLDIR); zip -r ../$(ZIPDIR)/web.zip *)

clean:
	rm -rf $(HTMLDIR)

$(HTMLDIR)/%.html: %.rd
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cat $(HEAD) `if [ -f $(dir $<)_nav.rd ]; then echo $(dir $<)_nav.rd; else echo ""; fi` $< $(FOOT) | $(RD2) -r rd/rd2html-ext-lib.rb --native-inline --ref-extension --headline-title --head-element --with-part=head --with-part=html:include > $@

$(HTMLDIR)/%.css: %.css
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cp $< $@

$(HTMLDIR)/images/%.png: images/%.png
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cp $< $@
