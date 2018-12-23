# Environment setup

In this chapter, I'll cover the basic environment setup to get started with ARM bare-metal programming using an emulator. Some familiarity with using Linux is assumed. You don't need to be a Linux expert, but you should be able to use the command line and do some basic troubleshooting if the system doesn't behave as expected.

## Linux

The first prerequisite is getting a Linux system up and running. Hopefully you are already familiar with Linux and have some kind of Linux running. Otherwise you should install Linux, or set up a virtual machine running Linux.

If you are running Windows and want to run a virtual Linux, [VirtualBox](http://www.virtualbox.org) is recommended. As for Linux distributions, any modern distribution should be fine, although in some cases you might need to install software manually. I use Linux Mint Debian Edition, and double-check most of the work in a virtual machine running Ubuntu, which is the most popular Linux distribution for beginners.

## QEMU

To emulate an ARM machine, we will be using [QEMU](http://www.qemu.org), a powerful emulation and virtualization tool that works with a variety of architectures. While the code we write should eventually be able to boot on a real ARM device, it is much easier to start with an emulator. Why?

* No additional hardware is needed.

* You don't have to worry about software flashing / download process.

* You have much better tools to inspect the state of the emulated hardware. When working with real hardware, you would need a few drivers to get meaningful information from the software, or use other more difficult methods.

Since QEMU supports a wide range of systems, we'll need to install the ARM version. On Debian/Ubuntu based systems, the `qemu-system-arm` package will provide what you need, so let's just go ahead and install it:

```
sudo apt-get install qemu-system-arm
```

## GCC cross-compiler toolchain

The next step is the installation of a cross-compiler toolchain. You cannot use the regular `gcc` compiler to build code that will run on an ARM system, instead you'll need a cross-compiler. What is a cross-compiler exactly? It's simply a compiler that runs on one platform but creates executables for another platform. In our case, we're running Linux on the x86-64 platform, and we want executables for ARM, so a cross compiler is the solution to that.

The GNU build tools, and by extension GCC, use the concept of a *target triplet* to describe a platform. The triplet lists the platform's architecture, vendor and operating system or binary interface type. The vendor part of target triplets is generally irrelevant. You can look up your own machine's target triplet by running `gcc -dumpmachine`. I get `x86_64-linux-gnu`, yours will likely be the same or similar.

To compile for ARM, we need to select the correct cross-compiler toolchain, that is, the toolchain with a target triplet matching our actual target. The fairly widespread `gcc-arm-linux-gnueabi` toolchain will **not** work for our needs, and you can probably guess why -- the name indicates that the toolchain is intended to compile code for ARM devices running Linux. We're going to do bare-metal programming, so no Linux running on the target system.

The toolchain we need is `gcc-arm-none-eabi`. We will need a version with GCC 6 or newer, for when we later use U-Boot. On Ubuntu, you should be able to simply install the toolchain:

```
sudo apt-get install gcc-arm-none-eabi
```

You can run `arm-none-eabi-gcc --version` to check the version number. If you're using a distribution that offers an old version of the package, you can download the toolchain [from ARM directly](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads). In that case, it's recommended that you add the toolchain's folder to your environment's `PATH` after extracting it somewhere.

## Build system essentials

Finally, we need the essential components of a build system. In the coming examples, we'll be using the standard Make build tool, as well as CMake. Debuan-based systems provide a handy package called `build-essential`, which installs Make and other relevant programs. CMake is available in a package called `cmake`, so the installation is simple:

```
sudo apt-get install build-essential cmake
```

On some Linux variants, you might also need to install `bison` and `flex` if they are not already present. Those tools are also required to build U-Boot.

```
sudo apt-get install bison flex
```

---

`sort -R ~/facts-and-trivia | head -n1 `

The `flex` program is an implementation of `lex`, a standard lexical analyzer first developed in the mid-1970s by Mike Lesk and Eric Schmidt, who served as the chairman of Google for some years.

---

With this, your system should now have everything that is necessary to compile programs for ARM and run them in an emulated machine. In the next chapter, we'll continue our introduction by booting the emulated machine and giving some of the just-installed tools a spin.
