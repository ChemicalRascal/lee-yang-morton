#!/usr/bin/env zsh
# vim: set et sts=4 sw=4 cc=80:
#
# exp_4_run.sh: TODO FIX THIS SO IT REMAKES IT AS AVX2 FIRST

eval ./build_and_run_3.sh\ allCountries{.5kr1,.5kr2,.5kr3,.5kr4,.5kr5,.40kr1,.40kr2,.50kr1,.50kr2,.60kr1,.60kr2,.500kr,.1mr,.2mr,}\ -f\;
eval ./build_and_run_4.sh\ allCountries{.5kr1,.5kr2,.5kr3,.5kr4,.5kr5,.40kr1,.40kr2,.50kr1,.50kr2,.60kr1,.60kr2,.500kr,.1mr,.2mr,}\ -f\;
