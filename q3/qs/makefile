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
RESES			= $(OBJDIR)/ui/$(PROJECTNAME)$(BASEPLATFORM).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x20000000

STLPORTEXPORT	= 1

include ../common.mak

$(OBJDIR)/ui/$(PROJECTNAME)$(BASEPLATFORM).res: $(SRCDIR)/ui/$(PROJECTNAME).rc $(SRCDIR)/ui/resource.h $(SRCDIR)/net/net.rc $(SRCDIR)/net/netresource.h
