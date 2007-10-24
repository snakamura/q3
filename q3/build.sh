#!/bin/sh
#
# $Id$


MAKE=${MAKE:=make}
VCDIR=${VCDIR:=C:/Program Files/Microsoft Visual Studio 8/VC}
MSDEV=${MSDEV:=C:/Program Files/Microsoft Visual Studio/Common/MSDev98/Bin/MSDEV.exe}
PURIFY=${PURIFY:=C:/Program Files/Rational/Purify/purify.exe}
MAKENSIS=${MAKENSIS:=C:/Program Files/NSIS/makensis.exe}
DOXYGEN=${DOXYGEN:=C:/Program Files/doxygen/bin/doxygen.exe}

INSTALLDIR="C:/Program Files/QMAIL3"

PROJECTS="qs qscrypto qsconvja qm qmpop3 qmimap4 qmsmtp qmnntp qmrss qmscript qmpgp qmjunk q3"
WINTARGETS="win.x86.unicode.release win.x64.unicode.release"
WCETARGETS="wm5.armv4i.ja"
#WINTARGETS="win.x86.unicode.release win.x86.ansi.release win.x64.unicode.release"
#WCETARGETS="wm5.armv4i.ja ppc2003se.armv4.ja ppc2003.armv4.ja sig3.armv4i.ja ppc2002.arm.ja hpc2000.arm.ja hpc2000.mips.ja ppc.arm.ja ppc.sh3.ja ppc.mips.ja hpcpro.arm.ja hpcpro.mips.ja hpcpro.sh3.ja hpcpro.sh4.ja"
TARGETS="$WINTARGETS $WCETARGETS"
MUIS="0411"

if [ $# -eq 0 ]; then
	COMMAND=all
else
	COMMAND=$1
fi

cpu() {
	echo $1 | sed -e 's/^[^.]\+\.\([^.]\+\)\..\+$/\1/'
}

basecpu() {
	c=`cpu $1`
	case $c in
	arm*)
		echo arm
		;;
	*)
		echo $c
		;;
	esac
}

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
	bindir=`pwd`/../bin
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
	cp ../lib/openssl/lib/win/x86/libeay32.dll "$INSTALLDIR"
	cp ../lib/openssl/lib/win/x86/ssleay32.dll "$INSTALLDIR"
	cp ../lib/stlport/lib/win/x86/stlport.5.1.dll "$INSTALLDIR"
	cp ../lib/zip/lib/win/x86/zip32.dll "$INSTALLDIR"
	cp ../LICENSE "$INSTALLDIR"
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
	DISTDIR=`pwd`/../dist
	SUFFIX=`printf $VERSION | tr . _`_$REVISION-$DATE
	
	mkdir -p $DISTDIR
	
#	zip -j $DISTDIR/q3-win-x86-ja-$SUFFIX.zip \
#		*/bin/win/x86/ansi/release/*.exe \
#		*/lib/win/x86/ansi/release/*.dll \
#		*/lib/win/x86/ansi/release/*.mui
	zip -j $DISTDIR/q3u-win-x86-ja-$SUFFIX.zip \
		*/bin/win/x86/unicode/release/*.exe \
		*/lib/win/x86/unicode/release/*.dll \
		*/lib/win/x86/unicode/release/*.mui \
		"$VCDIR/redist/x86/Microsoft.VC80.CRT/msvcr80.dll" \
		"$VCDIR/redist/x86/Microsoft.VC80.CRT/msvcp80.dll" \
		"$VCDIR/redist/x86/Microsoft.VC80.CRT/Microsoft.VC80.CRT.manifest" \
		../lib/stlport/lib/win/x86/stlport.5.1.dll \
		../lib/openssl/lib/win/x86/libeay32.dll \
		../lib/openssl/lib/win/x86/ssleay32.dll \
		../lib/zip/lib/win/x86/zip32.dll \
		../LICENSE
	zip -j $DISTDIR/q3u-win-x64-ja-$SUFFIX.zip \
		*/bin/win/x64/unicode/release/*.exe \
		*/lib/win/x64/unicode/release/*.dll \
		*/lib/win/x64/unicode/release/*.mui \
		"$VCDIR/redist/amd64/Microsoft.VC80.CRT/msvcr80.dll" \
		"$VCDIR/redist/amd64/Microsoft.VC80.CRT/msvcp80.dll" \
		"$VCDIR/redist/amd64/Microsoft.VC80.CRT/Microsoft.VC80.CRT.manifest" \
		../lib/stlport/lib/win/x64/stlport.5.1.dll \
		../lib/openssl/lib/win/x64/libeay32.dll \
		../lib/openssl/lib/win/x64/ssleay32.dll \
		../lib/zip/lib/win/x64/zip32.dll \
		../LICENSE
	
	for t in $WCETARGETS; do
		zip -j $DISTDIR/q3u-`printf $t | tr . -`-$SUFFIX.zip \
			*/bin/`printf $t | tr . /`/release/*.exe \
			*/lib/`printf $t | tr . /`/release/*.dll \
			*/lib/`printf $t | tr . /`/release/*.mui \
			"$VCDIR/ce/Dll/`cpu $t`/msvcr80.dll" \
			../lib/stlport/lib/wce/`basecpu $t`/stlport.5.1.dll \
			../lib/openssl/lib/wce/`basecpu $t`/libeay32.dll \
			../lib/openssl/lib/wce/`basecpu $t`/ssleay32.dll \
			../LICENSE
	done
	
	(cd ../docs; make zip)
	mv $DISTDIR/doc.zip $DISTDIR/q3-doc-$SUFFIX.zip
	;;

doc)
	(cd ../docs; make)
	;;

apidoc)
	"$DOXYGEN"
	;;

installer)
	"$MAKENSIS" ../installer/q3.nsi
	"$MAKENSIS" /DX64 ../installer/q3.nsi
#	"$MAKENSIS" /DANSI ../installer/q3.nsi
	
	VERSION=`cat version`
	REVISION=`cat revision`
	DATE=`date +%Y%m%d`
	DISTDIR=`pwd`/../dist
	mv $DISTDIR/q3u-win-x86-ja.exe $DISTDIR/q3u-win-x86-ja-`printf $VERSION | tr . _`_$REVISION-$DATE.exe
	mv $DISTDIR/q3u-win-x64-ja.exe $DISTDIR/q3u-win-x64-ja-`printf $VERSION | tr . _`_$REVISION-$DATE.exe
#	mv $DISTDIR/q3-win-x86-ja.exe $DISTDIR/q3-win-x86-ja-`printf $VERSION | tr . _`_$REVISION-$DATE.exe
    ;;

*)
	REVISION=`svn info --xml .. | xsltproc --param path "'info/entry/commit/@revision'" xpath.xsl -`
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
