# $Id: makefile,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
#
# Makefile for qmail

PROJECTNAME		= q3
PROJECTTYPE		= exe
LIBRARIES		= qs
ifdef DEBUG
	LIBRARIES	+= qm
endif


OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
RESES			= $(patsubst $(SRCDIR)/%.rc, $(OBJDIR)/%.res, $(wildcard $(SRCDIR)/*.rc))

include ../common.mak
