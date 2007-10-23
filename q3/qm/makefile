# $Id$
#
# Makefile for qm

include ../function.mak

ZIPDIR			= ../../lib/zip
DRADIR			= ../../lib/dra

PROJECTNAME		= qm
PROJECTTYPE		= dll
LIBRARIES		= qs

CPROJS			= -DQMEXPORT
ifeq ($(PLATFORM),win)
	CPROJS		+= -DQMHTMLVIEW -DQMHTMLVIEWWEBBROWSER -DQMTABWINDOW -DQMZIP -DQMTOOLTIP -DQMRECENTSWINDOW
endif
ifeq ($(PLATFORM),sig3)
	CPROJS		+= -DQMHTMLVIEW -DQMHTMLVIEWWEBBROWSER -DQMTABWINDOW
endif
ifneq ($(call platform,hpc2000 hpcpro),)
	CPROJS		+= -DQMTABWINDOW
endif
ifneq ($(call platform,wm5 ppc2003se ppc2003 ppc2002),)
	CPROJS		+= -DQMHTMLVIEW -DQMHTMLVIEWHTMLCTRL
endif

EXTERNALINCS	= -I"$(ZIPDIR)/include" -I"$(DRADIR)/include"

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*/*.cpp))
RESES			= $(OBJDIR)/ui/$(PROJECTNAME)$(BASEPLATFORM).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x30000000

include ../common.mak

%: %.sh

$(SRCDIR)/main/version.h: version revision
	( \
		source version.sh; \
		\
		printf "const int QMAIL_VERSION = %d%02d%03d;\n" \
			`cat version | tr '.' ' '`; \
		printf "const int QMAIL_REVISION = %d;\n" $(REVISION); \
		printf "\n"; \
		for r in $(SRCDIR)/ui/res/*.xml $(SRCDIR)/ui/res/*.template $(SRCDIR)/ui/res/*.bmp; do \
			printf "const int %s_REVISION = %d;\n" \
				`basename $$r | tr 'a-z.' 'A-Z_'` \
				`getNewestRevision $$r`; \
		done \
	) > $@

$(OBJDIR)/main/application.obj: $(SRCDIR)/main/version.h

$(OBJDIR)/ui/$(PROJECTNAME)$(BASEPLATFORM).res: $(SRCDIR)/ui/$(PROJECTNAME).rc $(SRCDIR)/ui/resource.h $(SRCDIR)/model/model.rc $(SRCDIR)/model/modelresource.h
