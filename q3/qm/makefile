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
ifeq ($(PLATFORM),desktop)
	RESES		= $(OBJDIR)/ui/$(PROJECTNAME).res
else
	RESES		= $(OBJDIR)/ui/$(PROJECTNAME)$(BASEPLATFORM).res
endif
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x30000000

include ../common.mak

$(SRCDIR)/main/version.h: version
	printf "const int QMAIL_VERSION = %d%02d%03d;\n" `cat version | tr '.' ' '` > $@

$(OBJDIR)/main/application.obj: $(SRCDIR)/main/version.h
