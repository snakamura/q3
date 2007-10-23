# $Id$
#
# Makefile for qmpgp

QDBMDIR			= ../../lib/qdbm


PROJECTNAME		= qmjunk
PROJECTTYPE		= dll
LIBRARIES		= qs qm

EXTERNALINCS	= -I"$(QDBMDIR)/include"
ifdef OLDWINDOWS
	EXLIBSUFFIX	= -vc6
endif
ifeq ($(PLATFORM),win)
	EXTERNALLIBS= "$(QDBMDIR)/lib/win/$(EXLIBCPU)/qdbm$(DSUFFIX)_static$(EXLIBSUFFIX).lib"
else
	EXTERNALLIBS= "$(QDBMDIR)/lib/wce/$(EXLIBCPU)/qdbm$(DSUFFIX)_static.lib"
endif

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
RESES			= $(OBJDIR)/$(PROJECTNAME).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x42000000

include ../common.mak
