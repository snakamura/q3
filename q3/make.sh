#!/bin/sh
#
# $Id$

MAKE=${MAKE:=make}

TARGET=$1

PLATFORMS="win wm6pro wm6std wm5 ppc2003se ppc2003 ppc2002 hpc2000 ppc hpcpro sig3"
WINCPUS="x86 x64"
WCECPUS="sh3 sh4 mips arm xscale armv4 armv4i x86em"
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

if [ "$PLATFORM" = "win" ]; then
	CPU=`echo $TARGET | cut -d . -f 2`
	checkValue "$CPU" "$WINCPUS" CPU
	BASELANG=ja
	CODE=`echo $TARGET | cut -d . -f 3`
	checkValue "$CODE" "$CODES" CODE
	DEBUG=`echo $TARGET | cut -d . -f 4`
	checkValue "$DEBUG" "$DEBUGS" DEBUG
else
	CPU=`echo $TARGET | cut -d . -f 2`
	checkValue "$CPU" "$WCECPUS" CPU
	BASELANG=`echo $TARGET | cut -d . -f 3`
	checkValue "$BASELANG" "$BASELANGS" BASELANG
	case $PLATFORM in
	wm6*)
		CEVER=502
		;;
	wm5)
		CEVER=501
		;;
	ppc2003se)
		CEVER=421
		;;
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
if [ "$CODE" = "ansi" ]; then
	OPTIONS="$OPTIONS OLDWINDOWS=1"
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

(cd ..; tar cf - version revision) | tar xf -
$MAKE target $OPTIONS
if [ $? -ne 0 ]; then
	exit 1
fi
for langid in src.*; do
	if [ $langid != "src.*" ]; then
		$MAKE target.mui MUILANG=${langid#src.} $OPTIONS
		if [ $? -ne 0 ]; then
			exit 1
		fi
	fi
done
if [ "$BSC" = "1" ]; then
	$MAKE target.bsc $OPTIONS
fi
