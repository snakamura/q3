# $Id$
#
# Makefile for qs

PROJECTNAME		= qs
PROJECTTYPE		= dll

CPROJS			= -DQSEXPORT
ifdef KCONVERT
	CPROJS		+= -DQS_KCONVERT
	KCTRL		= 1
endif

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*/*.cpp)) $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*/*.c))
ifeq ($(PLATFORM),desktop)
	RESES		= $(OBJDIR)/ui/$(PROJECTNAME).res
else
	RESES		= $(OBJDIR)/ui/$(PROJECTNAME)$(BASEPLATFORM).res
endif
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x20000000

STLPORTEXPORT	= 1

include ../common.mak
