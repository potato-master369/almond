#!/bin/sh

# Find available loop device, attach disk.img with partition scanning (-P), and print the device name
LOOP_DEV=$(losetup --find --show -P disk.img)

# Mount the first partition dynamically based on the assigned loop device
mount "${LOOP_DEV}p1" /mnt
cp -r rootfs/* /mnt/
umount /mnt

# Detach the correct loop device
losetup -d "$LOOP_DEV"
