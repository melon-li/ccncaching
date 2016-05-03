#!/bin/bash

FILE=$1 # topology
PRINTS=$2; # number of chunk_ids_to print
MODE=$3
LRU="LRU"
if [ "$MODE" = "LRU" ]; then 
	cat $FILE |  grep "(stored_items)" | awk '{print $6; print "\n"}' | tr , "\n" | tr - " "> /tmp/$FILE-proc;
	cat /tmp/$FILE-proc | awk -v var=$PRINTS 'BEGIN{for (x = 1; x <= var; x++) array[x]=0; }
												{ if ( $(NF) > var ) x = var; else x = $(NF);   array[x]++; }
											END{ for (x = 1; x <= var; x++) print array[x]; }'  > /tmp/$FILE-all								
else 
	cat $FILE |  grep "(stored_items)" | awk '{print $7; print "\n"}' | tr , "\n" | tr - " " > /tmp/$FILE-proc;
	cat /tmp/$FILE-proc | awk -v var=$PRINTS 'BEGIN{for (x = 1; x <= var; x++) array[x]=0; }
												{ if ( $(NF) > var ) { z = var;} else z = $(NF);   for (i = 1; i <= z; i++) array[i]++; if ( $(NF) > var ) array[z]+=$(NF)-var; }
											END{ for (x = 1; x <= var+1; x++) print array[x]
											}' > /tmp/$FILE-all									
fi					
			

for i in 1 5 10 50 100 500 1000 5000 10000  50000; do
	cat /tmp/$FILE-all | awk -v d=$i 'BEGIN{sum=0}{if (NR<=d) sum+=$1 }END{print sum}'
done
			  
rm /tmp/$FILE-proc;
rm /tmp/$FILE-all;
