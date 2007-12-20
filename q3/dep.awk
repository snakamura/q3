#!/bin/awk
#
# $Id$

{
	if (NR == 1) {
		print
	}
	else {
		for (i = 1; i <= NF; i++) {
			if ($i ~ /^include\// ||
				$i ~ /^src.*\// ||
				$i ~ /^obj.*\// ||
				$i ~ /^\.\.\// ||
				$i == "\\")
			printf " " $i
		}
		print ""
	}
}
