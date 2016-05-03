#!/bin/bash

LOG=$1 # topology
FILE_LIST=$2; # number of chunk_ids_to print
MODE=$3
LRU="LRU"
if [ "$MODE" = "LRU" ]; then 
	cat $LOG |  grep "(stored_items)" | awk '{print $6; print "\n"}' | tr , "\n" | tr - " " | awk '{print $1"_ "$2}' > /tmp/$LOG-proc;
    for i in `cat $FILE_LIST`; do  cat /tmp/$LOG-proc | grep "/"$i"_" | awk 'BEGIN{sum=0}{sum++;}END{print sum}'; done

else 
	cat $LOG |  grep "(stored_items)" | awk '{print $7; print "\n"}' | tr , "\n" | tr - " " | awk '{print $1"_ "$2}' > /tmp/$LOG-proc;
    for i in `cat $FILE_LIST`; do  cat /tmp/$LOG-proc | grep "/"$i"_" | awk 'BEGIN{sum=0}{sum+=$2}END{print sum}'; done

fi					
	
		


rm /tmp/$LOG-proc		  

