#!/bin/env sh
# vim: set et sts=4 sw=4 cc=80 tw=80: 
#
# run_tests.sh: Using the python scripts, run all possible queries on a
#               randomly-generated tree of size $1 and a point probability
#               of $2, and compare the python output with the "real" output.

PREFIX=data/$(date "+%Y.%m.%d.%T")
MAX=$(python -c "print(2**$1 - 1)")

mkdir -p data
touch $PREFIX.queries &&
time (for lox in $(seq 0 $MAX);
do
    for hix in $(seq $lox $MAX);
    do
        for loy in $(seq 0 $MAX);
        do
            for hiy in $(seq $loy $MAX);
            do
                echo "$lox $loy $hix $hiy"
            done
        done
    done
done) >> $PREFIX.queries &&
echo "queries made"
