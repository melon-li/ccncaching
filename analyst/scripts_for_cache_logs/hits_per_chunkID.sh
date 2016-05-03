#!/bin/bash

FILE=$1 # topology
PRINTS=$2; # number of chunk_ids_to print
cat $FILE |  grep "(chunkID_hits)" | awk '{print $7; print "\n"}' | tr , "\n" | tr : " " > /tmp/$FILE-proc;

cat /tmp/$FILE-proc | awk -v var=$PRINTS 'BEGIN{for (x = 1; x <= var; x++) array[x]=0; }
												{ if ( $1 > var ) x = var; else x = $1;   array[x]+=$2}
											END{ for (x = 1; x <= var; x++) print array[x]; }' > /tmp/$FILE-all
										  

for i in 1 5 10 50 100 500 1000 5000 10000 50000; do
	cat /tmp/$FILE-all | awk -v d=$i 'BEGIN{sum=0}{if (NR<=d) sum+=$1 }END{print sum}'
done
			  
rm /tmp/$FILE-proc;
rm /tmp/$FILE-all;
