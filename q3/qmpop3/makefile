# $Id$
#
# Makefile for qmpop3

PROJECTNAME		= qmpop3
PROJECTTYPE		= dll
LIBRARIES		= qs qm

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
RESES			= $(OBJDIR)/$(PROJECTNAME)$(BASEPLATFORM).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x37000000

include ../common.mak
