#!/usr/bin/env sh
# vim: set et sts=4 sw=4 cc=80 tw=80: 
#
# build_and_run.sh: For when you only want to type out the damn args once.
# 
#                 $1:  $PREFIX
#                 $2+: List of "modes" ("-c n" as "-cn")

./prep_Binaries_3.sh $@ &&
./prep_Binaries_4.sh $@ &&
./run_timings_3_4.sh $@
