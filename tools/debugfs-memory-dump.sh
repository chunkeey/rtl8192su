#!/bin/bash

let end=0x10000
cat $1 > $2
for ((i = 0; i < $end; i+=4)); do
	echo "$i of $end"
	while ((1)); do
		printf "0x%x 4" $i > $1 && break
	done
	tail -n1 $1 >> $2
done
