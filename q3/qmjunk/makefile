# $Id$
#
# Makefile for qmpgp

QDBMDIR			= d:/dev/qdbm/qdbm


PROJECTNAME		= qmjunk
PROJECTTYPE		= dll
LIBRARIES		= qs qm

EXTERNALINCS	= -I"$(QDBMDIR)/include"
ifeq ($(PLATFORM),desktop)
	EXTERNALLIBS= "$(QDBMDIR)/lib/desktop/qdbm_t$(DSUFFIX).lib"
else
	EXTERNALLIBS= "$(QDBMDIR)/lib/wce/$(EXLIBCPU)/qdbm_t$(DSUFFIX).lib"
endif

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
RESES			= $(OBJDIR)/$(PROJECTNAME)$(BASEPLATFORM).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x42000000

include ../common.mak
