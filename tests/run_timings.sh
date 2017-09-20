#!/usr/bin/env sh
# vim: set et sts=4 sw=4 cc=80 tw=80: 
#
# run_timings.sh: Runs timing code against $PREFIX.queries_$SUFFIX -es.
# 
#                 $1:  $PREFIX
#                 $2+: List of "modes" ("-c n" as "-cn")

PREFIX=$1

for i in `ls -1 $PREFIX.queries_*`; do
    echo "$i"
    SUFFIX=${i#$PREFIX.queries_}
    echo "$SUFFIX" > $PREFIX.res
    for j in ${@#$1}; do
        echo $j
        ./../qtree_testbench -qtx "$PREFIX" $j < $i > $PREFIX.res
    done
    echo "----" > $PREFIX.res
    true
done
