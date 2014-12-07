#!/bin/bash 

PATH=..
# Prerequisite:
#   './user -a' is executed on both ends.

duration=30  # total experiment duration
interval=5         # interval between each sample
tmp=0

echo "Start Experiment for $duration sec"

while [ $tmp -lt $duration ]; do
    $PATH/user -e 1
    /bin/sleep $interval
    tmp=$((tmp+interval))
    echo $tmp
done
