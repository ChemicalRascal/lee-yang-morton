#!/usr/bin/env sh
# vim: set et sts=4 sw=4 cc=80 tw=80: 
#
# prep_Queries.sh
#
#   $1:  $PREFIX
#   $2:  Number of sets to generate
#   $3+: Number of queries per set (avoid duplicates!)

v=${@#$1}
v=${v#$2}
for i in $v; do
    for j in `seq 1 $2`; do
        ./../gen_queries_percent -x $1 -f ${1}.queries_${i}_${j} -g -3 -h 3 -i 0.01 -n $i
    done
done
