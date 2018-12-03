#!/usr/bin/env bash

sudo apt-get install qemu-system-arm gcc-arm-none-eabi build-essential cmake bison flex

if [[ $? > 0 ]]; then
    echo "Installation failed! Check connectivity and apt-get setup"
    exit 1
fi

VER=$(arm-none-eabi-gcc -dumpversion)
MAJOR_VER=${VER%%.*}
echo $MAJOR_VER

if [[ "$MAJOR_VER" < 6 ]]; then
    echo "GCC for ARM installed but is too old! At least version 6 is needed"
    exit 1
fi
