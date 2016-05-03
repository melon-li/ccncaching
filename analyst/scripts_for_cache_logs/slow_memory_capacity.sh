#!/bin/bash

FILE=$1 # topology
NODES=$2;
cat $FILE | grep "(stored_items)" | awk -v nd=$NODES 'BEGIN{for (x=0; x <nd ; x++) array[x]=0;}{array[NR%nd]+=$6}END{for (x=0; x <nd ; x++) print array[x]/(int(NR/nd));}'
