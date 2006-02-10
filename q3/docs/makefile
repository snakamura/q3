HTMLDIR	= html

HTMLS	= $(patsubst %.rd, $(HTMLDIR)/%.html, $(wildcard [A-Z]*.rd))
HEAD	= head.rd

all: $(HTMLS)

$(HTMLDIR)/%.html: %.rd
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	cat $(HEAD) $< | ruby -Ku /usr/local/bin/rd2 -r rd/rd2html-ext-lib.rb --ref-extension --headline-secno --head-element --with-part=head > $@
