# $Id$
#
# Makefile for qscrypto

OPENSSLDIR		= ../lib/openssl
WCECOMPATDIR	= ../lib/wcecompat


PROJECTNAME		= qscrypto
PROJECTTYPE		= dll
LIBRARIES		= qs

EXTERNALINCS	= -I"$(OPENSSLDIR)/include" -I"$(WCECOMPATDIR)/include"
ifeq ($(PLATFORM),desktop)
	EXTERNALLIBS= "$(OPENSSLDIR)/lib/win/libeay32.lib" "$(OPENSSLDIR)/lib/win/ssleay32.lib"
else
	EXTERNALLIBS= "$(OPENSSLDIR)/lib/wce/$(EXLIBCPU)/libeay32.lib" "$(OPENSSLDIR)/lib/wce/$(EXLIBCPU)/ssleay32.lib"
endif

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
RESES			= $(OBJDIR)/$(PROJECTNAME).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x25000000

include ../common.mak
