#!/usr/bin/env zsh
# vim: set et sts=4 sw=4 cc=80:
#
# insert_sizes.sh: Fix the csvs missing info
#
#               $@: The csvs

for i in $@; do
    sed -i -e 's/^,0/,size,0/' $i
    for prefix in `awk -F ',' '/^,/{f=1;next} /----/{exit 0} f { print $1 }' $i`; do
        fpre=${prefix/avx2/qsi}
        #echo $prefix $fpre
        size=`cat sizes | grep ${i%.res.csv}.$fpre | awk '{print $1}'`
        sed -i -e "s/^${prefix},/$prefix,$size/" $i
    done
done
