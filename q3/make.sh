#!/bin/sh
#
# $Id$

MAKE=${MAKE:=make}

TARGET=$1

PLATFORMS="desktop ppc2003 ppc2002 hpc2000 ppc hpcpro sig3"
CPUS="sh3 sh4 mips arm xscale armv4 armv4i x86em"
BASELANGS="ja en"
CODES="ansi unicode"
DEBUGS="release debug"

checkValue()
{
	for v in $2; do
		if [ "$1" = "$v" ]; then
			return 0
		fi
	done
	
	echo Unknown $3: $1
	exit 1
}

PLATFORM=`echo $TARGET | cut -d . -f 1`
checkValue "$PLATFORM" "$PLATFORMS" PLATFORM

if [ "$PLATFORM" = "desktop" ]; then
	BASELANG=ja
	CODE=`echo $TARGET | cut -d . -f 2`
	checkValue "$CODE" "$CODES" CODE
	DEBUG=`echo $TARGET | cut -d . -f 3`
	checkValue "$DEBUG" "$DEBUGS" DEBUG
else
	CPU=`echo $TARGET | cut -d . -f 2`
	checkValue "$CPU" "$CPUS" CPU
	BASELANG=`echo $TARGET | cut -d . -f 3`
	checkValue "$BASELANG" "$BASELANGS" BASELANG
	case $PLATFORM in
	ppc2003)
		CEVER=420
		;;
	ppc2002 | hpc2000 | ppc)
		CEVER=300
		;;
	hpcpro)
		CEVER=211
		;;
	sig3)
		CEVER=410
		;;
	*)
		echo "Unknown PLATFORM: $PLATFORM"
		exit 1
		;;
	esac
fi

OPTIONS="PLATFORM=$PLATFORM"
if [ "$CPU" != "" ]; then
	OPTIONS="$OPTIONS CPU=$CPU"
fi
if [ "$CEVER" != "" ]; then
	OPTIONS="$OPTIONS CEVER=$CEVER"
fi
OPTIONS="$OPTIONS BASELANG=$BASELANG"
if [ "$CODE" != "" ]; then
	OPTIONS="$OPTIONS CODE=$CODE"
fi
if [ "$BASELANG" != "ja" ]; then
	OPTIONS="$OPTIONS KCONVERT=1"
fi
if [ "$CPU" = "x86em" ]; then
	OPTIONS="$OPTIONS EMULATION=1"
fi
if [ "$DEBUG" = "debug" ]; then
	OPTIONS="$OPTIONS DEBUG=1"
fi

(cd ..; tar cf - version) | tar xf -
$MAKE target $OPTIONS
