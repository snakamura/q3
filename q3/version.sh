#!/bin/bash
#
# $Id: $

VERSION=`echo $1 | cut -d . -f 4`

YEAR=$((2000 + ($VERSION >> 11)))
MONTH=$((($VERSION & 0x780) >> 7))
DAY=$((($VERSION & 0xae) >> 2))
EXTRA=$(($VERSION & 0x3))

printf "%s %02d/%02d/%02d-%d\n" $1 $YEAR $MONTH $DAY $EXTRA
