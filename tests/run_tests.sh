#!/bin/env sh
# vim: set et sts=4 sw=4 cc=80 tw=80: 
#
# run_tests.sh: Using the python scripts, run all possible queries on a
#               randomly-generated tree of size $1 and a point probability
#               of $2, and compare the python output with the "real" output.

PREFIX=data/$(date "+%Y.%m.%d.%T")
MAX=$(python -c "print(2**$1 - 1)")

mkdir -p data
touch $PREFIX.csv
./gen_tree_input.py $1 $2 > $PREFIX.csv &&
echo "csv made" &&
./../qtree_testbench -t $PREFIX.tree.bin -f $PREFIX.csv -b &&
echo "bin made" &&
touch $PREFIX.queries &&
./../gen_queries -d $1 -f $PREFIX.queries &&
echo "queries made" &&
./../qtree_testbench -t $PREFIX.tree.bin < $PREFIX.queries > $PREFIX.qt_out &&
echo "qt_out made" &&
./query_tree.py $PREFIX.csv < $PREFIX.queries > $PREFIX.py_out &&
echo "py_out made" &&
diff $PREFIX.qt_out $PREFIX.py_out
