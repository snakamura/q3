#!/bin/sh
#
# $Id$


MAKE=${MAKE:=make}
MSDEV=${MSDEV:=d:/dev/msvs/common/msdev98/bin/msdev.exe}
PURIFY=${PURIFY:=d:/dev/rational/purify/purify.exe}

PROJECTS="qs qscrypto qm qmpop3 qmimap4 qmsmtp qmnntp qmrss qmscript q3"
DESKTOPTARGETS="desktop.ansi.release desktop.unicode.release"
CETARGETS="ppc2003.armv4.ja ppc2002.arm.ja hpc2000.arm.ja hpc2000.mips.ja ppc.arm.ja ppc.sh3.ja ppc.mips.ja hpcpro.arm.ja hpcpro.mips.ja hpcpro.sh3.ja hpcpro.sh4.ja sig3.armv4i.ja"
TARGETS="$DESKTOPTARGETS $CETARGETS"

if [ $# -eq 0 ]; then
	COMMAND=all
else
	COMMAND=$1
fi

case $COMMAND in
clean | clean.desktop | clean.wce)
	for p in $PROJECTS; do
		cd $p
		$MAKE $COMMAND
		cd ..
	done
	;;

copy)
	bindir=`pwd`/bin
	mkdir -p $bindir
	for p in $PROJECTS; do
		for dir in bin lib; do
			if [ -d $p/$dir ]; then
				cd $p/$dir
				tar cf - `/bin/find . \( -name *.exe -o -name *.dll \)` | (cd $bindir; tar xf -)
				cd ../..
			fi
		done
	done
	;;

run|run.unicode|run.debug|run.debug.unicode|debug|debug.unicode|purify|purify.unicode)
	case $COMMAND in
	run)
		CODE=ansi
		DEBUG=release
		SUFFIX=
		EXEC=
		;;
	run.unicode)
		CODE=unicode
		DEBUG=release
		SUFFIX=u
		EXEC=
		;;
	run.debug)
		CODE=ansi
		DEBUG=debug
		SUFFIX=d
		EXEC=
		;;
	run.debug.unicode)
		CODE=unicode
		DEBUG=debug
		SUFFIX=ud
		EXEC=
		;;
	debug)
		CODE=ansi
		DEBUG=debug
		SUFFIX=d
		EXEC=$MSDEV
		;;
	debug.unicode)
		CODE=unicode
		DEBUG=debug
		SUFFIX=ud
		EXEC=$MSDEV
		;;
	purify)
		CODE=ansi
		DEBUG=debug
		SUFFIX=d
		EXEC=$PURIFY
		;;
	purify.unicode)
		CODE=unicode
		DEBUG=debug
		SUFFIX=ud
		EXEC=$PURIFY
		;;
	esac
	
	RUNPATH=`pwd`/lib
	for p in $PROJECTS; do
		RUNPATH="$RUNPATH:`pwd`/$p/lib/desktop/$CODE/$DEBUG"
	done
	
	PATH="$PATH:$RUNPATH" $EXEC q3/bin/desktop/$CODE/$DEBUG/q3$SUFFIX.exe &
	;;

countline)
	wc `/bin/find . -regex ".*/\(include\|src\)/.*\.\(h\|cpp\|inl\|idl\|rc\)$"` | sort
	;;

countclass)
	(for f in `/bin/find . -regex ".*\.\(h\|cpp\|inl\)$"`; do
		cat $f | tr -d '\r' | grep '^\(class\|struct\) .*[^;>]$'
	done) | wc
	;;

checksize)
	size=0
	for f in `/bin/find . -regex ".*[^d]\.\(exe\|dll\)" -printf "%s "`; do
		size=`expr $size + $f`
	done
	echo `expr $size / 1024`K
	;;

all)
	for t in $TARGETS; do
		./build.sh $t
	done
	;;

zip)
	VERSION=`cat version`
	DATE=`date +%Y%m%d`
	ZIPDIR=./zip
	
	mkdir -p $ZIPDIR
	
	zip -j $ZIPDIR/q3-desktop-x86-ja-`printf $VERSION | tr . _`-$DATE.zip \
		*/bin/desktop/ansi/release/*.exe \
		*/lib/desktop/ansi/release/*.dll
	zip -j $ZIPDIR/q3u-desktop-x86-ja-`printf $VERSION | tr . _`-$DATE.zip \
		*/bin/desktop/unicode/release/*.exe \
		*/lib/desktop/unicode/release/*.dll
	
	for t in $CETARGETS; do
		zip -j $ZIPDIR/q3u-`printf $t | tr . -`-`printf $VERSION | tr . _`-$DATE.zip \
			*/bin/`printf $t | tr . /`/release/*.exe \
			*/lib/`printf $t | tr . /`/release/*.dll
	done
	;;

ziptool)
	VERSION=`cat version`
	DATE=`date +%Y%m%d`
	ZIPDIR=./zip
	
	mkdir -p $ZIPDIR
	
	zip $ZIPDIR/tool-`printf $VERSION | tr . _`-$DATE.zip `/bin/find tool -regex ".*\(hta\|js\|xsl\)"`
	;;

*)
	REVISION=`svn info | grep Revision | cut -d ' ' -f 2`
	if [ -f revision ]; then
		OLDREVISION=`cat revision`
	fi
	if [ "$REVISION" != "$OLDREVISION" ]; then
		echo $REVISION > revision
	fi
	for p in $PROJECTS; do
		cd $p
		$MAKE $COMMAND
		if [ $? -ne 0 ]; then
			exit 1
		fi
		cd ..
	done
	;;
esac
