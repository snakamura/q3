#!/bin/awk
#
# $Id: $

BEGIN {
	if (LIBNAME == "") {
		print "LIBNAME must be set." > "/dev/stderr"
		exit 1;
	}
	print "LIBRARY " LIBNAME;
	print "EXPORTS";
	DO=0;
}
{
	if (DO == 0) {
		if ($1 == "ordinal")
			DO = 1;
	}
	else if (DO == 1 && $1 != "") {
		if ($1 == "Summary")
			DO = 2;
		else
			print "\t" $4 " @ " $1 " NONAME";
	}
}
