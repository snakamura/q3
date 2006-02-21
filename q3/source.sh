#!/bin/sh
#
# $Id$

ARCHIVE=d:/home/src/q3-`pwd | sed -e 's#.*/\([^/]*\)/[^/]*#\1#'`.zip

GLOBALS=" \
	version \
	*.mak \
	*.sh \
	*.awk \
	*.xsl \
	patch/*.patch \
"
SRC=`/bin/find q* \( \( -type f -a -regex ".*/\(src\|include\)\(/.*\.\(cpp\|c\|h\|inl\|idl\|def\|rc\)\|.*/res/.*\)$" -a ! -regex ".*/\.svn/.*" \) -o -name makefile -o -name libname -o -name platforms \) -a ! -name version.h`

SCHEMA=" \
	`/bin/find schema -name "*.rnc"` \
"

DOCS=" \
	Doxyfile \
	docs/makefile \
	docs/*.rd \
	docs/images/*.png \
"
zip -ru $ARCHIVE $GLOBALS $SRC $SCHEMA $DOCS
