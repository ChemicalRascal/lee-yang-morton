#!/usr/bin/env zsh
# vim: set et sts=4 sw=4 cc=80:
#
# csvify.sh: Take those merged experiments, yeah? Now make 'em nicer to graph.
#
#               $1: Folders the results are in.

e="----"

IFS=$'\t\n\0'
for file in $1/*res; do
    rm $file.csv

    #header line
    for l in `grep -h "|Q|" $file`; do
        echo $l | awk -F '_' '/_1/{ printf ",%s", $1 }' >> $file.csv
    done
    echo "," >> $file.csv

    #blah
    echo $file
    for prefix in `awk '/Q/{f=1;next} /----/{exit 0} f { print $1 }' $file`; do
        echo $prefix
        printf $prefix >> $file.csv
        #for l in `grep -h "$prefix" $file`; do
        #    echo $l | awk '{ printf ",%s", $2 }' >> $file.csv
        #done
        awk "BEGIN {s=0} /$prefix/{s+=\$2;next} /0.01_1/{next} /_1, /{printf \",%f\",s;s=0}" $file >> $file.csv
        echo "," >> $file.csv
    done
done
