# Bare-metal C programming on ARM

This repository contains a tutorial ebook concerning programming a bare-metal ARM system. More specifically it deals with a ARMv7A version of the ARM Versatile Express platform, emulated on a regular PC through QEMU. You can explore the repository, or read things in order.

## Table of Contents

An up-to-date PDF version is [also available](http://umanovskis.se/files/arm-baremetal-ebook.pdf).

This is still a work in progress. Currently available:

* [Chapter 0](doc/00_introduction.md): Introduction. A brief intro to the subject and the ebook.
* [Chapter 1](doc/01_setup.md): Setup. A short chapter dealing with preparing a Linux environment for further development.
* [Chapter 2](doc/02_first_boot.md): The first boot. Basic use of QEMU and the cross-compiler toolchain, getting the simplest possible code to run.
* [Chapter 3](doc/03_bootloader.md): Adding a bootloader. Building the highly popular U-Boot bootloader, and getting it to boot our own code.
* [Chapter 4](doc/04_cenv.md): Preparing a C environment. This chapter deals with the necessary work for getting from startup in assembly code to C code.
* [Chapter 5](doc/05_cmake.md): Build & debug system. Here we show how the work can be streamlined by adding a CMake-based build system, and how the bare-metal program can be debugged.
* [Chapter 6](doc/06_uart.md): UART driver development. In this chapter, a device driver for a UART gets written.
* Chapter 7. Currently WIP.

## Repository structure

The repository consists of two top-level folders. The `doc` folder contains the actual tutorial chapters. The `src` folder contains the source code corresponding to each chapter. So, for instance, [src/04_cenv](src/04_cenv) contains the source code as it looks after completing Chapter 4.

Additionally, the `src` folder has some shared things. `src/common_uboot` holds a stripped-down version of U-Boot used in the examples.

Have fun, and feel free to tweak and experiment, that being a great way to learn!
