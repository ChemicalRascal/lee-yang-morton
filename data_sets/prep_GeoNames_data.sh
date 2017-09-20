#!/usr/bin/env sh
# vim: set et sts=4 sw=4 cc=80 tw=80: 
#
# prepGeoNames.sh
#
#   $1: Name of GeoNames dataset (<$1>.txt)

awk -F '\t' '{printf("%f,%f\n", $6, $5)}' < $1.txt | sort -k1,1n -k2,2n -t, |\
uniq > $1.csv_raw &&
./../chazelle_reduce -x $1
