# $Id$
#
# Makefile for qmpop4

PROJECTNAME		= qmimap4
PROJECTTYPE		= dll
LIBRARIES		= qs qm

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
ifeq ($(PLATFORM),desktop)
	RESES		= $(OBJDIR)/$(PROJECTNAME).res
else
	RESES		= $(OBJDIR)/$(PROJECTNAME)$(BASEPLATFORM).res
endif
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x35000000

include ../common.mak
