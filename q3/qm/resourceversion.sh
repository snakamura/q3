#!/bin/sh
#
# $Id$

srcdir=$1
baseplatform=$2

basesrcdir=${srcdir%%\.*}

cat <<END
#include <qmresourceversion.h>

const qm::ResourceVersion resourceVersions[] = {
END

for r in $basesrcdir/ui/res/*.xml $basesrcdir/ui/res/*.template $basesrcdir/ui/res/*.bmp; do
	b=$(basename $r)
	lpr=$srcdir/ui/res/$baseplatform/$b
	lr=$srcdir/ui/res/$b
	pr=$(dirname $r)/$baseplatform/$b
	if [ -f $lpr ]; then
		r=$lpr
	elif [ -f $lr ]; then
		r=$lr
	elif [ -f $pr ]; then
		r=$pr
	fi
	
	printf "\t{ %d, %d },\n" $(LC_MESSAGES=C svn info $r | grep "Last Changed Rev" | cut -f 4 -d ' ') $(wc -c $r | sed -e 's/^ *\([0-9]\+\).*$/\1/')
done

cat <<END
};
END
