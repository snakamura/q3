# $Id$
#
# Makefile for qm

PROJECTNAME		= qm
PROJECTTYPE		= dll
LIBRARIES		= qs
EXTRAINCLUDES	= qmscript

CPROJS			= -DQMEXPORT
ifeq ($(PLATFORM),desktop)
	CPROJS		+= -DQMHTMLVIEW
endif
ifeq ($(PLATFORM),sig3)
	CPROJS		+= -DQMHTMLVIEW
endif

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*/*.cpp))
RESES			= $(OBJDIR)/ui/$(PROJECTNAME)$(BASEPLATFORM).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x30000000

include ../common.mak

$(SRCDIR)/main/version.h: version revision
	( \
		printf "const int QMAIL_VERSION = %d%02d%03d;\n" \
			`cat version | tr '.' ' '`; \
		printf "const int QMAIL_REVISION = %d;\n" $(REVISION); \
		printf "\n"; \
		for r in $(SRCDIR)/ui/res/*.xml $(SRCDIR)/ui/res/*.template $(SRCDIR)/ui/res/*.bmp; do \
			printf "const int %s_REVISION = %d;\n" \
				`basename $$r | tr 'a-z.' 'A-Z_'` \
				`svn info $$r | grep "Last Changed Rev" | cut -f 4 -d ' '`; \
		done \
	) > $@

$(OBJDIR)/main/application.obj: $(SRCDIR)/main/version.h
