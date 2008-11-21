# $Id$

SHELL			= /bin/sh

UNAME			= $(shell uname | sed -e 's/_.*//')

cever			= $(if $(CEVER),$(shell test "$(CEVER)" $(1) "$(2)"; echo $$?),1)
platform		= $(if $(filter $(1),$(PLATFORM)),$(PLATFORM),)
vcver			= $(if $(VCVER),$(shell test "$(VCVER)" $(1) "$(2)"; echo $$?),1)
ifeq ($(UNAME),CYGWIN)
	win2unix	= $(if $(1),$(shell cygpath -u "$(1)"),)
endif
ifeq ($(UNAME),MINGW32)
	win2unix	= $(if $(1),$(shell echo "$(1)" | sed -e 's/\([A-Za-z]\):/\/\1/'),)
endif
