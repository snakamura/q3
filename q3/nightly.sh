#!/bin/sh
#
# $Id$


SVNDIR=d:/home/svn
CGIFTPSCRIPT=d:/home/wince/q3/cgiftpscript
WEBFTPSCRIPT=d:/home/wince/q3/webftpscript

svnadmin dump $SVNDIR/q3 | gzip -c > $SVNDIR/q3-`date +%Y%m%d`.gz

if [ "$1" = "-norebuild" ]; then
	REBUILD=0
else
	REBUILD=1
fi


VERSION=`cat version`
DATE=`date +%Y%m%d`

BUILDDIR=`pwd`/build
ZIPDIR=$BUILDDIR/zip

if [ $REBUILD -ne 0 ]; then
	rm -rf $BUILDDIR
fi

if [ ! -d $BUILDDIR ]; then
	mkdir $BUILDDIR
fi
if [ ! -d $ZIPDIR ]; then
	mkdir $ZIPDIR
fi

cd $BUILDDIR
svn checkout file:///home/svn/q3/trunk/q3
cd q3

make all

zip -j $ZIPDIR/q3-desktop-x86-ja-$VERSION-$DATE.zip \
	*/bin/desktop/ansi/release/*.exe \
	*/lib/desktop/ansi/release/*.dll
zip -j $ZIPDIR/q3u-desktop-x86-ja-$VERSION-$DATE.zip \
	*/bin/desktop/unicode/release/*.exe \
	*/lib/desktop/unicode/release/*.dll

PLATFORMS="ppc2002.arm.ja hpc2000.arm.ja hpc2000.mips.ja ppc.arm.ja ppc.mips.ja ppc.sh3.ja hpcpro.arm.ja hpcpro.mips.ja hpcpro.sh3.ja hpcpro.sh4.ja"
for p in $PLATFORMS; do
	zip -j $ZIPDIR/q3u-`printf $p | tr . -`-$VERSION-$DATE.zip \
		*/bin/`printf $p | tr . /`/release/*.exe \
		*/lib/`printf $p | tr . /`/release/*.dll
done


#ncftpput -f $WEBFTPUSER /omega/snak/software/q3/nightly $ZIPDIR/*
(cd $ZIPDIR; ftp -i -s:$WEBFTPSCRIPT)

echo $VERSION-$DATE > nightly
#ncftpput -f $CGIFTPUSER /public_html/q3 nightly
ftp -s:$CGIFTPSCRIPT
