# $Id: makefile,v 1.3 2003/05/30 06:22:57 snakamura Exp $
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
	EXTERNALLIBS= $(OPENSSLDIR)/lib/desktop/libeay32.lib
else
	EXTERNALLIBS= $(OPENSSLDIR)/lib/wce/$(CPU)/libeay32.lib
endif

OBJS			= $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.obj, $(wildcard $(SRCDIR)/*.cpp))
RESES			= $(OBJDIR)/$(PROJECTNAME).res
DEFFILE			= $(OBJDIR)/$(PROJECTNAME).def
BASEADDRESS		= 0x25000000

include ../common.mak
