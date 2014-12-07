#!/bin/sh
module="stp"
device="altera_dma"
mode="666"

# load the module

if [ `lsmod | grep -o $module` ]; then
    /sbin/rmmod $module
fi
/sbin/insmod ./$module.ko $* || exit 1

# remove stale nodes
rm -f /dev/$device

# create new device node
major=`grep -w $device /proc/devices | cut -f1 -d" "`
mknod /dev/$device c $major 0

# change permissions to allow all users to read/write
chmod $mode /dev/$device

# make user program
#rm -f ./ser/user
#gcc ./user/user.c -o ./user/user
