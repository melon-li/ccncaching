#!/bin/bash

LOG=$1 
FILE_LIST=$2
MODE=$3
LRU="LRU"
if [ "$MODE" = "LRU" ]; then 
	cat $LOG |  grep "(file_hits)" | awk '{print $6; print "\n"}' | tr , "\n" |  tr : " "> /tmp/$LOG-hits_per_file				
						
else 
	cat $LOG |  grep "(file_hits)" | awk '{print $6; print "\n"}' | tr , "\n" | tr : " "> /tmp/$LOG-hits_per_file		
fi					


for i in `cat $FILE_LIST`; do cat /tmp/$LOG-hits_per_file | grep "domain1/$i-" | awk 'BEGIN{sum=0;}{sum+=$2}END{print sum}'; done
