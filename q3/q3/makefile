# $Id$
#
# Makefile for qmail

PROJECTNAME		= q3
PROJECTTYPE		= exe
LIBRARIES		= qs qm


OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
RESES			= $(patsubst $(SRCDIR)/%.rc, $(OBJDIR)/%.res, $(wildcard $(SRCDIR)/*.rc))

include ../common.mak
