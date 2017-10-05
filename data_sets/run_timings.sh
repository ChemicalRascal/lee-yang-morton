#!/usr/bin/env sh
# vim: set et sts=4 sw=4 cc=80 tw=80: 
#
# run_timings.sh: Runs timing code against $PREFIX.queries_$SUFFIX -es.
# 
#                 $1:  $PREFIX
#                 $2+: List of "modes" ("-c n" as "-cn")

PREFIX=$1
DATESTRING=`date +%Y.%m.%d.%T`

S_T=`date -u +"%s"`
for i in `ls -1 $PREFIX.queries_*`; do
    n=`wc -l $i | cut -f1 -d' '`
    echo "Running $i: $n queries"
    SUFFIX=${i#$PREFIX.queries_}
    echo "$SUFFIX, |Q|: $n" >> $PREFIX.res_$DATESTRING
    for j in ${@#$1}; do
        echo "On $j"
        ./../qtree_testbench -qtx "$PREFIX" $j < $i >> $PREFIX.res_$DATESTRING
    done
    echo "----" >> $PREFIX.res_$DATESTRING
    true
done
E_T=`date -u +"%s"`
D_T=$((E_T-S_T))
echo "$((D_T/60))m, $((D_T%60))s" >> $PREFIX.res_$DATESTRING
