#!/bin/bash

rm -f results.csv

for i in $(seq 5 5 100)
do
    ./virtmem 100 $i custom sort
done
