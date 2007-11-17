#!/bin/sh
#
# $Id$

getRevision() {
	LC_MESSAGES=C svn info $1 | grep "Last Changed Rev" | cut -f 4 -d ' '
}

getNewestPlatformRevision() {
	rev=`getRevision $1`
	for p in hpc ppc; do
		x=`dirname $1`/$p/`basename $1`
		if [ -f $x ]; then
			revx=`getRevision $x`
			if [ $rev -lt $revx ]; then
				rev=$revx;
			fi
		fi
	done
	echo $rev
}

getNewestRevision() {
	rev=`getNewestPlatformRevision $1`
	base=`echo $1 | sed -e 's%/.*%%'`
	rest=`echo $1 | sed -e 's%[^/]*/%%'`
	for l in $base.*/$rest; do
		if [ -f $l ]; then
			revx=`getNewestPlatformRevision $l`
			if [ $rev -lt $revx ]; then
				rev=$revx
			fi
		fi
	done
	echo $rev
}

getSize() {
	wc -c $1 | cut -d ' ' -f 1
}
