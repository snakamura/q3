# $Id$
#
# Makefile for qmpop3

PROJECTNAME		= qmnntp
PROJECTTYPE		= dll
LIBRARIES		= qs qm

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
RESES			= $(OBJDIR)/$(PROJECTNAME)$(BASEPLATFORM).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x36000000

include ../common.mak
