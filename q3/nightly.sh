#!/bin/sh
#
# $Id$


SVNHOST=home.snak.org
SVNDIR=/home/snakamura/svn
SVNURI=svn+ssh://$SVNHOST$SVNDIR/q3/trunk/q3
CGIFTPSCRIPT=d:/home/wince/q3/cgiftpscript
WEBFTPSCRIPT=d:/home/wince/q3/webftpscript

DATE=`date +%Y%m%d`
ssh $SVNHOST "svnadmin dump $SVNDIR/q3 | gzip -c > $SVNDIR/q3-$DATE.gz"
scp $SVNHOST:$SVNDIR/q3-$DATE.gz /cygdrive/d/home/svn


if [ "$1" = "-norebuild" ]; then
	REBUILD=0
else
	REBUILD=1
fi

VERSION=`cat version`
DATE=`date +%Y%m%d`

BUILDDIR=`pwd`/build
ZIPDIR=$BUILDDIR/q3/zip

if [ $REBUILD -ne 0 ]; then
	rm -rf $BUILDDIR
fi

if [ ! -d $BUILDDIR ]; then
	mkdir $BUILDDIR
fi

cd $BUILDDIR
svn checkout $SVNURI
cd q3

make all NODEPEND=1
make zip ziptool


(cd $ZIPDIR; ftp -i -s:$WEBFTPSCRIPT)

echo $VERSION-$DATE > nightly
ftp -s:$CGIFTPSCRIPT
