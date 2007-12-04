# $Id$
#
# Makefile for qs

DRADIR			= ../../lib/dra

PROJECTNAME		= qs
PROJECTTYPE		= dll

CPROJS			= -DQSEXPORT
ifdef KCONVERT
	CPROJS		+= -DQS_KCONVERT
	KCTRL		= 1
endif
ifneq ($(call platform,wm6std),)
	CPROJS		+= -DQSDIALOGMENU
endif

EXTERNALINCS	= -I"$(DRADIR)/include"

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*/*.cpp)) $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*/*.c))
RESES			= $(OBJDIR)/ui/$(PROJECTNAME)$(BASEPLATFORM).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x20000000

include ../common.mak

$(OBJDIR)/ui/$(PROJECTNAME)$(BASEPLATFORM).res: $(SRCDIR)/ui/$(PROJECTNAME).rc $(SRCDIR)/ui/resource.h $(SRCDIR)/net/net.rc $(SRCDIR)/net/netresource.h
