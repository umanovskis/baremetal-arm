#!/usr/bin/env bash

arm-none-eabi-as -o startup.o startup.s
arm-none-eabi-ld -T linkscript.ld -o better-hang.elf startup.o
arm-none-eabi-objcopy -O binary better-hang.elf better-hang.bin
