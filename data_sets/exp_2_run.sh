#!/usr/bin/env zsh
# vim: set et sts=4 sw=4 cc=80:
#
# exp_run.sh: Just do everything, for a given definition of everything.

eval ./build_and_run.sh\ allCountries\ -c\{1024,1280,1536,1792,2048\}\;
