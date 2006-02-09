HTMLDIR	= html

HTMLS	= $(patsubst %.rd, $(HTMLDIR)/%.html, $(wildcard *.rd))

all: $(HTMLS)

$(HTMLDIR)/%.html: %.rd
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	rd2 -r rd/rd2html-ext-lib.rb --ref-extension --headline-secno $< > $@
