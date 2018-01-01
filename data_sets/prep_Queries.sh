#!/usr/bin/env sh
# vim: set et sts=4 sw=4: 
#
# prep_Queries.sh
#
#   $1:  $PREFIX
#   $2:  Number of sets to generate
#   $3:  Number of queries per set
#   $4+: Size of each query (avoid duplicates)

v=${@#$1}
v=${v#" "}
v=${v#$2}
v=${v#" "}
v=${v#$3}
v=${v#" "}
for i in $v; do
    for j in `seq 1 $2`; do
        ./../gen_queries_percent -x $1 -f ${1}.queries_${i}_${j} -i $i -n $3
    done
done
