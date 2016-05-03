#!/bin/bash

FILE=$1 # topology
PRINTS=51; # number of chunk_ids_to print

cat $FILE |  grep "(file_hits)" | awk '{print $7; print "\n"}' | tr , "\n" | tr : " " > /tmp/$FILE-proc;

cat /tmp/$FILE-proc | awk -v var=$PRINTS 'BEGIN{for (x = 1; x <= var; x++) array[x]=0; }
												{ if ( $1 > var ) x = var; else x = $1;   array[x]+=$2}
											END{ for (x = 1; x <= var; x++) print array[x]; }';
										  
rm /tmp/$FILE-proc;
