# $Id$
#
# Makefile for qscrypto

OPENSSLDIR		= d:/dev/openssl/openssl
WCECOMPATDIR	= d:/dev/wcecompat/wcecompat


PROJECTNAME		= qscrypto
PROJECTTYPE		= dll
LIBRARIES		= qs

CPROJS			= -DQSCRYPTOEXPORT

EXTERNALINCS	= -I$(OPENSSLDIR)/include -I$(WCECOMPATDIR)/include
ifeq ($(PLATFORM),desktop)
	EXTERNALLIBS= $(OPENSSLDIR)/lib/desktop/libeay32.lib $(OPENSSLDIR)/lib/desktop/ssleay32.lib
else
	EXTERNALLIBS= $(OPENSSLDIR)/lib/wce/$(CPU)/libeay32.lib $(OPENSSLDIR)/lib/wce/$(CPU)/ssleay32.lib
endif

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
RESES			= $(OBJDIR)/$(PROJECTNAME).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x25000000

include ../common.mak
