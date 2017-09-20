#!/usr/bin/env sh
# vim: set et sts=4 sw=4 cc=80 tw=80: 
#
# run_tests.sh: Using the python scripts, run all possible queries on a
#               randomly-generated tree.
#
#   $1: Min x
#   $2: Max x
#   $3: Min y
#   $4: Max y
#   $5: Point probability
#   $6: Size of queries as a percent of the total area of the space
#   $7: Number of queries generated

PREFIX=data/$(date "+%Y.%m.%d.%T")
MAX=$(python -c "print(2**$1 - 1)")
Q=16

mkdir -p data
touch $PREFIX.csv
./gen_tree_input.py $1 $2 $3 $4 $5 > $PREFIX.csv &&
echo "csv made" &&
./../qtree_testbench -x $PREFIX -b -f -g -q &&
echo "bin made" &&
./../qtree_testbench -x $PREFIX -b -c $Q -q &&
echo "qsi made" &&
touch $PREFIX.queries &&
#./../gen_queries -x $PREFIX -f $PREFIX.queries &&
./../gen_queries_percent -x $PREFIX -f $PREFIX.queries -g -3 -h 3 -i $6 -n $7 &&
echo "queries made" &&
./../qtree_testbench -x $PREFIX -g < $PREFIX.queries > $PREFIX.ofb_out &&
echo "ofb_out made" &&
./../qtree_testbench -x $PREFIX -f < $PREFIX.queries > $PREFIX.k2_out &&
echo "k2_out made" &&
./../qtree_testbench -x $PREFIX -qc $Q \
                < $PREFIX.queries > $PREFIX.qt_${Q}_out &&
echo "qt_out made" &&
./query_tree.py $PREFIX.csv < $PREFIX.queries > $PREFIX.py_out &&
echo "py_out made"
echo "ofb diff:"
diff $PREFIX.ofb_out $PREFIX.py_out
echo "k2 diff:"
diff $PREFIX.k2_out $PREFIX.py_out
echo "qt diff:"
diff $PREFIX.qt_${Q}_out $PREFIX.py_out
