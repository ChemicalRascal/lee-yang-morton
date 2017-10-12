#!/usr/bin/env zsh
# vim: set et sts=4 sw=4 cc=80:
#
# exp_3_run.sh: TODO FIX THIS SO IT REMAKES IT AS AVX2 FIRST

eval ./build_and_run.sh\ allCountries{.5kr1,.5kr2,.5kr3,.5kr4,.5kr5,.40kr1,.40kr2,.50kr1,.50kr2,.60kr1,.60kr2,.500kr,.1mr,.2mr,}\ -c\{32,64,128,256,512,1024,1280,1536,1792,2048\}\;
