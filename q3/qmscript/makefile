# $Id$
#
# Makefile for qmscript

PROJECTNAME		= qmscript
PROJECTTYPE		= dll
LIBRARIES		= qs qm

CPROJS			= -DQMSCRIPTEXPORT

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
RESES			= $(patsubst $(SRCDIR)/%.rc, $(OBJDIR)/%.res, $(wildcard $(SRCDIR)/*.rc))
TLBS			= $(patsubst $(SRCDIR)/%.idl, $(TLBDIR)/%.tlb, $(wildcard $(SRCDIR)/*.idl))
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
EXTRADEFFILE	= $(SRCDIR)/qmscript.def
BASEADDRESS		= 0x38000000

include ../common.mak
