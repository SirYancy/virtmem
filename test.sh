#!/bin/bash

rm -f *results.csv

algorithm=rand


for i in $(seq 5 5 100)
do
    ./virtmem 100 $i $algorithm sort
done

for i in $(seq 5 5 100)
do
    ./virtmem 100 $i $algorithm scan
done

for i in $(seq 5 5 100)
do
    ./virtmem 100 $i $algorithm focus
done
