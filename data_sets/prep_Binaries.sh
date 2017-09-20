#!/usr/bin/env sh
# vim: set et sts=4 sw=4 cc=80 tw=80: 
#
# prep_Binaries.sh
#
#   $1: $PREFIX
#   $2+: List of "modes" ("-c n" as "-cn")

#-c 16 -c 32 -c 512 -c 2000 -c 20000 -f -g -q
./../qtree_testbench -bqx $1 -b ${@#$1}
