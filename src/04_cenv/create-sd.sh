#!/bin/bash

SDNAME="$1"
UIMGNAME="$2"

if [ "$#" -ne 2 ]; then
    echo "Usage: "$0" sdimage uimage"
    exit 1
fi

command -v qemu-img >/dev/null || { echo "qemu-img not installed"; exit 1; }
command -v qemu-nbd >/dev/null || { echo "qemu-nbd not installed"; exit 1; }

qemu-img create "$SDNAME" 64M
sudo qemu-nbd -c /dev/nbd0 "$SDNAME"
(echo o;
echo n; echo p
echo 1
echo ; echo
echo w; echo p) | sudo fdisk /dev/nbd0
sudo mkfs.ext2 /dev/nbd0p1

mkdir tmp || true
sudo mount -o user /dev/nbd0p1 tmp/
sudo cp "$UIMGNAME" tmp/
sudo umount /dev/nbd0p1
rmdir tmp || true
sudo qemu-nbd -d /dev/nbd0
