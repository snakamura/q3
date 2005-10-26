# $Id: makefile 2214 2005-07-25 01:35:10Z snakamura $
#
# Makefile for qsconvja

PROJECTNAME		= qsconvja
PROJECTTYPE		= dll
LIBRARIES		= qs
KCTRL			= 1

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
RESES			= $(OBJDIR)/$(PROJECTNAME).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x27000000

include ../common.mak
