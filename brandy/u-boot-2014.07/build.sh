#!/bin/bash

echo para is $1
if [ $1 == boot0 -o  $1 == spl -o $1 == fes ]; then 
	#make clean
	make -f spl_make $1
else
	echo make $1
	make  $1
fi


