# $Id$
#
# Makefile for qscrypto

OPENSSLDIR		= ../lib/openssl
WCECOMPATDIR	= ../lib/wcecompat

PROJECTNAME		= qscrypto
PROJECTTYPE		= dll
LIBRARIES		= qs

EXTERNALINCS	= -I"$(OPENSSLDIR)/include" -I"$(WCECOMPATDIR)/include"
ifeq ($(PLATFORM),win)
	EXTERNALLIBS= "$(OPENSSLDIR)/lib/win/$(EXLIBCPU)/libeay32$(DSUFFIX).lib" "$(OPENSSLDIR)/lib/win/$(EXLIBCPU)/ssleay32$(DSUFFIX).lib"
else
	EXTERNALLIBS= "$(OPENSSLDIR)/lib/wce/$(EXLIBCPU)/libeay32$(DSUFFIX).lib" "$(OPENSSLDIR)/lib/wce/$(EXLIBCPU)/ssleay32$(DSUFFIX).lib"
endif

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
RESES			= $(OBJDIR)/$(PROJECTNAME).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x25000000

include ../common.mak
