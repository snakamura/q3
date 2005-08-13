# $Id$

SHELL					= /bin/bash

cever					= $(if $(CEVER),$(shell test "$(CEVER)" $(1) "$(2)"; echo $$?),1)
platform				= $(if $(filter $(1),$(PLATFORM)),$(PLATFORM),)
win2unix				= $(if $(1),$(shell cygpath -u "$(1)"),)