#!/usr/bin/env sh
# vim: set et sts=4 sw=4 cc=80:
#
# full_sequence.sh: Just do everything.
# 
#                 $1:  $PREFIX
#                 $2+: List of "modes" ("-c n" as "-cn")

# csv from csv_raw
./../chazelle_reduce -x $1 &&
# min/max_x/y, maxatt
./../qtree_testbench -bx $1 &&
./prep_Queries.sh $1 8 1000 0.01 0.02 0.05 0.10 0.15 0.20 0.25 0.30 0.35 0.40 0.45 0.50 0.55 0.60 0.65 0.70 0.75 0.80
./build_and_run.sh $@
