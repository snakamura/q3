# $Id: makefile,v 1.2 2003/05/30 06:22:56 snakamura Exp $
#
# Makefile for qmpop3

PROJECTNAME		= qmpop3
PROJECTTYPE		= dll
LIBRARIES		= qs qm

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
ifeq ($(PLATFORM),desktop)
	RESES		= $(OBJDIR)/$(PROJECTNAME).res
else
	RESES		= $(OBJDIR)/$(PROJECTNAME)$(BASEPLATFORM).res
endif
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x37000000

include ../common.mak
