#!/usr/bin/env sh
# vim: set et sts=4 sw=4 cc=80 tw=80: 
#
# clear_results.sh: A poor substitute for make clean
# 
#                 $1:  $PREFIX

PREFIX=$1

rm $PREFIX.k2
rm $PREFIX.maxatt
rm $PREFIX.max_x
rm $PREFIX.max_y
rm $PREFIX.min_x
rm $PREFIX.min_y
rm $PREFIX.ofb
rm $PREFIX.qsi_*
rm $PREFIX.res_*
rm $PREFIX.queries_*
