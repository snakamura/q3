# $Id$
#
# Makefile for qmrss

PROJECTNAME		= qmrss
PROJECTTYPE		= dll
LIBRARIES		= qs qm

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
RESES			= $(OBJDIR)/$(PROJECTNAME)$(BASEPLATFORM).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x40000000

include ../common.mak
