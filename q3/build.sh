#!/bin/sh
#
# $Id: $


MAKE=${MAKE:=make}
MSDEV=${MSDEV:=d:/dev/msvs/common/msdev98/bin/msdev.exe}
PURIFY=${PURIFY:=d:/dev/rational/purify/purify.exe}

PROJECTS="qs qscrypto qm qmpop3 qmimap4 qmsmtp qmnntp qmscript q3"
TARGETS="desktop.ansi.release desktop.unicode.release ppc2002.arm.ja hpc2000.arm.ja hpc2000.mips.ja ppc.arm.ja ppc.sh3.ja ppc.mips.ja hpcpro.arm.ja hpcpro.mips.ja hpcpro.sh3.ja hpcpro.sh4.ja"

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

run|run.unicode|debug|debug.unicode|purify|purify.unicode)
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
	
	RUNPATH=`cygpath -w \`pwd\``/lib
	for p in $PROJECTS; do
		RUNPATH="$RUNPATH;`cygpath -w \`pwd\``/$p/lib/desktop/$CODE/$DEBUG"
	done
	
	PATH="$PATH;$RUNPATH" $EXEC q3/bin/desktop/$CODE/$DEBUG/q3$SUFFIX.exe &
	;;

countline)
	wc `/bin/find . -regex ".*/\(include\|src\)/.*\.\(h\|cpp\|inl\|idl\)$" -a ! -regex ".*/build/.*"` | sort
	;;

countclass)
	(for f in `/bin/find . -regex ".*\.\(h\|cpp\|inl\)$" -a ! -regex ".*/build/.*"`; do
		cat $$f | tr -d '\r' | grep '^\(class\|struct\) .*[^;>]$$'
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

*)
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
