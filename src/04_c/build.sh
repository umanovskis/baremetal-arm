#!/usr/bin/env bash

arm-none-eabi-as -g -o startup.o startup.s
arm-none-eabi-gcc -c -g -nodefaultlibs -nostdlib -nostartfiles -o cstart.o cstart.c
arm-none-eabi-ld -T linkscript.ld -o cenv.elf startup.o cstart.o

make -C ../common_uboot/ vexpress_ca9x4_config ARCH=arm CROSS_COMPILE=arm-none-eabi-
make -C ../common_uboot/ all ARCH=arm CROSS_COMPILE=arm-none-eabi-

../common_uboot/tools/mkimage -A arm -C none -T kernel -a 0x60000000 -e 0x60000000 -d cenv.elf bare-arm.uimg
./create-sd.sh sdcard.img bare-arm.uimg
