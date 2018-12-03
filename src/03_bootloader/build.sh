#!/usr/bin/env bash

arm-none-eabi-as -o startup.o startup.s
arm-none-eabi-ld -T linkscript.ld -o better-hang.elf startup.o
arm-none-eabi-objcopy -O binary better-hang.elf better-hang.bin

make -C ../common_uboot/ vexpress_ca9x4_config ARCH=arm CROSS_COMPILE=arm-none-eabi-
make -C ../common_uboot/ all ARCH=arm CROSS_COMPILE=arm-none-eabi-

../common_uboot/tools/mkimage -A arm -C none -T kernel -a 0x60000000 -e 0x60000000 -d better-hang.elf bare-arm.uimg
./create-sd.sh sdcard.img bare-arm.uimg
