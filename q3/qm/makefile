# $Id$
#
# Makefile for qm

include ../function.mak

ZIPDIR			= ../lib/zip

PROJECTNAME		= qm
PROJECTTYPE		= dll
LIBRARIES		= qs

CPROJS			= -DQMEXPORT
ifeq ($(PLATFORM),win)
	CPROJS		+= -DQMHTMLVIEW -DQMHTMLVIEWWEBBROWSER -DQMTABWINDOW -DQMZIP
endif
ifeq ($(PLATFORM),sig3)
	CPROJS		+= -DQMHTMLVIEW -DQMHTMLVIEWWEBBROWSER -DQMTABWINDOW
endif
ifneq ($(call platform, hpcpro hpc2000),)
	CPROJS		+= -DQMTABWINDOW
endif
ifneq ($(call platform, ppc2003 ppc2003se ppc2002),)
	CPROJS		+= -DQMHTMLVIEW -DQMHTMLVIEWHTMLCTRL
endif

EXTERNALINCS	= -I"$(ZIPDIR)/include"

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*/*.cpp))
RESES			= $(OBJDIR)/ui/$(PROJECTNAME)$(BASEPLATFORM).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x30000000

include ../common.mak

$(SRCDIR)/main/version.h: version revision
	( \
		getRevision() { \
			LC_MESSAGES=C svn info $$1 | grep "Last Changed Rev" | cut -f 4 -d ' '; \
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

$(OBJDIR)/ui/qm.res: $(SRCDIR)/ui/actionid.h $(SRCDIR)/model/model.rc $(SRCDIR)/model/modelresource.h
$(OBJDIR)/ui/qmhpc.res: $(SRCDIR)/ui/actionid.h $(SRCDIR)/model/model.rc $(SRCDIR)/model/modelresource.h
$(OBJDIR)/ui/qmppc.res: $(SRCDIR)/ui/actionid.h $(SRCDIR)/model/model.rc $(SRCDIR)/model/modelresource.h
