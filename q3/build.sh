#!/bin/sh
#
# $Id$


MAKE=${MAKE:=make}
MSDEV=${MSDEV:=C:/Program Files/Microsoft Visual Studio/Common/MSDev98/Bin/MSDEV.exe}
PURIFY=${PURIFY:=C:/Program Files/Rational/Purify/purify.exe}
MAKENSIS=${MAKENSIS:=C:/Program Files/NSIS/makensis.exe}
DOXYGEN=${DOXYGEN:=C:/Program Files/doxygen/doxygen.exe}

INSTALLDIR="C:/Program Files/QMAIL3"

PROJECTS="qs qscrypto qsconvja qm qmpop3 qmimap4 qmsmtp qmnntp qmrss qmscript qmpgp qmjunk q3"
WINTARGETS="win.x86.unicode.release win.x86.ansi.release win.x64.unicode.release"
WCETARGETS="wm5.armv4i.ja ppc2003se.armv4.ja ppc2003.armv4.ja sig3.armv4i.ja ppc2002.arm.ja hpc2000.arm.ja hpc2000.mips.ja ppc.arm.ja ppc.sh3.ja ppc.mips.ja hpcpro.arm.ja hpcpro.mips.ja hpcpro.sh3.ja hpcpro.sh4.ja"
TARGETS="$WINTARGETS $WCETARGETS"
MUIS="0411"

if [ $# -eq 0 ]; then
	COMMAND=all
else
	COMMAND=$1
fi

case $COMMAND in
clean | clean.win | clean.wce)
	for p in $PROJECTS; do
		cd $p
		$MAKE $COMMAND 
		for mui in $MUIS; do
			$MAKE $COMMAND MUILANG=$mui
		done
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
				tar cf - `/bin/find . ! -regex "\./lib/.*" -a \( -name *.exe -o -name *.dll -o -name *.mui \)` | (cd $bindir; tar xf -)
				cd ../..
			fi
		done
	done
	;;

install)
	cp */bin/win/x86/unicode/release/*.exe */lib/win/x86/unicode/release/*.dll "$INSTALLDIR"
	;;

install-mui)
	for mui in $MUIS; do
		cp */lib/win/x86/unicode/release/*.mui "$INSTALLDIR"
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
		RUNPATH="$RUNPATH:`pwd`/$p/lib/win/x86/$CODE/$DEBUG"
	done
	
	PATH="$PATH:$RUNPATH" "$EXEC" q3/bin/win/x86/$CODE/$DEBUG/q3$SUFFIX.exe &
	;;

countline)
	wc `/bin/find . ! -regex "\./lib/.*" -a -regex ".*/\(include\|src\(\..+\)?\)/.*\.\(h\|cpp\|inl\|idl\|rc\|xml\)$"` | sort
	;;

countclass)
	(for f in `/bin/find . ! -regex "\./lib/.*" -a -regex ".*\.\(h\|cpp\|inl\)$"`; do
		cat $f | tr -d '\r' | grep '^\(class\|struct\) .*[^;>]$'
	done) | wc
	;;

checksize)
	size=0
	for f in `/bin/find . ! -regex "\./lib/.*" -a -regex ".*[^d]\.\(exe\|dll\)" -printf "%s "`; do
		size=`expr $size + $f`
	done
	echo `expr $size / 1024`K
	;;

all)
	for t in $TARGETS; do
		./build.sh $t
	done
	;;

win)
	for t in $WINTARGETS; do
		./build.sh $t
	done
	;;

wce)
	for t in $WCETARGETS; do
		./build.sh $t
	done
	;;

zip)
	VERSION=`cat version`
	REVISION=`cat revision`
	DATE=`date +%Y%m%d`
	ZIPDIR=./zip
	SUFFIX=`printf $VERSION | tr . _`_$REVISION-$DATE
	
	mkdir -p $ZIPDIR
	
	zip -j $ZIPDIR/q3-win-x86-ja-$SUFFIX.zip \
		*/bin/win/x86/ansi/release/*.exe \
		*/lib/win/x86/ansi/release/*.dll
	zip -j $ZIPDIR/q3u-win-x86-ja-$SUFFIX.zip \
		*/bin/win/x86/unicode/release/*.exe \
		*/lib/win/x86/unicode/release/*.dll
	zip -j $ZIPDIR/q3u-win-x64-ja-$SUFFIX.zip \
		*/bin/win/x64/unicode/release/*.exe \
		*/lib/win/x64/unicode/release/*.dll
	for mui in $MUIS; do
		zip -j $ZIPDIR/q3-win-x86-ja-mui$mui-$SUFFIX.zip \
			*/lib/win/x86/ansi/release/*.mui
		zip -j $ZIPDIR/q3u-win-x86-ja-mui$mui-$SUFFIX.zip \
			*/lib/win/x86/unicode/release/*.mui
		zip -j $ZIPDIR/q3u-win-x64-ja-mui$mui-$SUFFIX.zip \
			*/lib/win/x64/unicode/release/*.mui
	done
	
	for t in $WCETARGETS; do
		zip -j $ZIPDIR/q3u-`printf $t | tr . -`-$SUFFIX.zip \
			*/bin/`printf $t | tr . /`/release/*.exe \
			*/lib/`printf $t | tr . /`/release/*.dll
		for mui in $MUIS; do
			zip -j $ZIPDIR/q3u-`printf $t | tr . -`-mui$mui-$SUFFIX.zip \
				*/lib/`printf $t | tr . /`/release/*.mui
		done
	done
	
	(cd docs; make zip)
	mv $ZIPDIR/doc.zip $ZIPDIR/q3-doc-$SUFFIX.zip
	;;

doc)
	(cd docs; make)
	;;

apidoc)
	"$DOXYGEN"
	;;

installer)
	"$MAKENSIS" installer/q3.nsi
	"$MAKENSIS" /DANSI installer/q3.nsi
	
	VERSION=`cat version`
	REVISION=`cat revision`
	DATE=`date +%Y%m%d`
	mv installer/q3u-win-x86-ja.exe installer/q3u-win-x86-ja-`printf $VERSION | tr . _`_$REVISION-$DATE.exe
	mv installer/q3-win-x86-ja.exe installer/q3-win-x86-ja-`printf $VERSION | tr . _`_$REVISION-$DATE.exe
    ;;

*)
	REVISION=`svn info --xml | xsltproc --param path "'info/entry/commit/@revision'" xpath.xsl -`
	if [ -f revision ]; then
		OLDREVISION=`cat revision`
	fi
	if [ "$REVISION" != "$OLDREVISION" ]; then
		echo $REVISION > revision
	fi
	for p in $PROJECTS; do
		cd $p
		if [ ! -f platforms ] || grep $COMMAND platforms; then
			$MAKE $COMMAND
			if [ $? -ne 0 ]; then
				exit 1
			fi
		fi
		cd ..
	done
	;;
esac
