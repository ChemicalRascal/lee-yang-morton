#!/usr/bin/env zsh
# vim: set et sts=4 sw=4 cc=80:
#
# merge_exps.sh: Take all those experiments, yeah? Now mush 'em together.
#
#               $1+: Folders the experiments are in.

dir="exp_merge/"
mkdir -p $dir

e="----"

IFS=$'\t\n\0'
for file in $1/*res_*; do
    #echo $file
    file_base=${${file#*/}%_*}
    out=$dir/$file_base
    for l in `grep -h "|Q|" $file`; do
        echo $l >> $out
        l=${l//|/\\|}
        for a in $@; do
            awk "/$l/{f=1;next} /$e/{f=0} f" $a/${file_base}_* >> $out
        done
        echo "\n" >> $out
    done
done
