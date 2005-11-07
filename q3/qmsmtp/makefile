# $Id$
#
# Makefile for qmsmtp

PROJECTNAME		= qmsmtp
PROJECTTYPE		= dll
LIBRARIES		= qs qm

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
RESES			= $(OBJDIR)/$(PROJECTNAME)$(BASEPLATFORM).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x39000000

include ../common.mak

$(OBJDIR)/$(PROJECTNAME)$(BASEPLATFORM).res: $(SRCDIR)/$(PROJECTNAME).rc $(SRCDIR)/resource.h
