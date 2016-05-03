#!/bin/bash

CATALOG=$1 
FILE_LIST=$2

cat $FILE_LIST | awk '{print "#"$1"#"}' > $FILE_LIST-tmp

for i in `cat $FILE_LIST-tmp`; do  cat $CATALOG | grep $i | awk '{print $3/1500" "$2}'; done

rm $FILE_LIST-tmp;
