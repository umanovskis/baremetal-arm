#!/usr/bin/env bash

(cd src/02_first_boot && ./build.sh)
(cd src/02_first_boot/better_hang && ./build.sh)

(cd src/03_bootloader && ./build.sh)

(cd src/04_cenv && ./build.sh)
(cd src/05_cmake && cmake -S . -Bbuild && cd build && make)
(cd src/06_uart &&  cmake -S . -Bbuild && cd build && make)
