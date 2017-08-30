#!/bin/env sh
# vim: set et sts=4 sw=4 cc=80 tw=80: 
#
# run_tests.sh: Using the python scripts, run all possible queries on a
#               randomly-generated tree of size $1 and a point probability
#               of $2, and compare the python output with the "real" output.

PREFIX=data/$(date "+%Y.%m.%d.%T")
MAX=$(python -c "print(2**$1 - 1)")
Q=16

mkdir -p data
touch $PREFIX.csv
./gen_tree_input.py $1 $2 > $PREFIX.csv &&
echo "csv made" &&
./../qtree_testbench -x $PREFIX -b -e -c $Q -f -q &&
echo "bin made" &&
touch $PREFIX.queries &&
./../gen_queries_percent -x $PREFIX -f $PREFIX.queries -g -3 -h 3 -i $3 -n $4 &&
echo "queries made" &&
./../qtree_testbench -x $PREFIX -c $Q < $PREFIX.queries > $PREFIX.qt_${Q}_out &&
echo "qt_out made" &&
./query_tree.py $PREFIX.csv < $PREFIX.queries > $PREFIX.py_out &&
echo "py_out made" &&
diff $PREFIX.qt_${Q}_out $PREFIX.py_out &&
./../qtree_testbench -x $PREFIX -f < $PREFIX.queries > $PREFIX.k2_out &&
echo "k2_out made" &&
echo "k2 diff:" &&
diff $PREFIX.k2_out $PREFIX.py_out #&&
echo "qt diff:" &&
diff $PREFIX.qt_${Q}_out $PREFIX.py_out &&
true
