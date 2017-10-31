#!/bin/bash

for i in $(seq 5 5 100)
do
    ./virtmem 100 $i rand focus
done
