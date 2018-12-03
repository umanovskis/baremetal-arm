#!/usr/bin/env bash

arm-none-eabi-as -o startup.o startup.s
arm-none-eabi-ld -T linkscript.ld -o first-hang.elf startup.o
arm-none-eabi-objcopy -O binary first-hang.elf first-hang.bin
