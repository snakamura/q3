#!/bin/sh

ARCHIVE=d:/temp/q3.zip

GLOBALS=" \
	version \
	*.mak \
	*.sh \
	*.awk \
	patch/*.patch \
"
SRC=`/bin/find . \( \( -type f -a -regex ".*/\(src\|include\)\(/.*\.\(cpp\|c\|h\|inl\|idl\|def\|rc\)\|.*/res/.*\)$" -a ! -regex ".*/CVS/.*" \) -o -name makefile -o -name libname \) -a ! -regex ".*/build/.*" -a ! -name version.h`

SCHEMA=" \
	`/bin/find schema -name "*.rnc"` \
"

DOCS=" \
"
SPRS=" \
"

KITS=" \
	kit \
"

zip -ru $ARCHIVE $GLOBALS $SRC $SCHEMA $KITS
