# $Id: makefile,v 1.2 2003/05/30 06:22:57 snakamura Exp $
#
# Makefile for qmsmtp

PROJECTNAME		= qmsmtp
PROJECTTYPE		= dll
LIBRARIES		= qs qm

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
ifeq ($(PLATFORM),desktop)
	RESES		= $(OBJDIR)/$(PROJECTNAME).res
else
	RESES		= $(OBJDIR)/$(PROJECTNAME)$(BASEPLATFORM).res
endif
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x39000000

include ../common.mak
