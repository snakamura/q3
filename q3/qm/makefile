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
		getRevision() { \
			svn info $$1 | grep "Last Changed Rev" | cut -f 4 -d ' '; \
		}; \
		getNewestRevision() { \
			rev=`getRevision $$1`; \
			for p in hpc ppc; do \
				x=`dirname $$1`/$$p/`basename $$1`; \
				if [ -f $$x ]; then \
					revx=`getRevision $$x`; \
					if [ $$rev -lt $$revx ]; then \
						rev=$$revx; \
					fi; \
				fi; \
			done; \
			echo $$rev; \
		}; \
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
