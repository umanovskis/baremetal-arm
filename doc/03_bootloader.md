# Adding a bootloader

## Introduction

The *bootloader* is a critical piece of software that is necessary to get the hardware into a usable state, and load other more useful programs, such as an operating system. On PCs and other fully-featured devices, common bootloaders include GNU GRUB (which is most likely used to boot your Linux system), bootmgr (for modern versions of MS Windows) and others. Developing bootloaders is a separate and complicated subject. Bootloaders are generally full of esoteric, highly architecture-specific code, and in my opinion learning about bootloaders if fun, but the knowledge is also somewhat less transferable to other areas of development.

Writing your own bootloader is certainly something that could be attempted, but in this series of posts we will continue by doing something that is usually done in embedded development, namely using Das U-Boot.

[Das U-Boot](https://www.denx.de/wiki/U-Boot/), usually referred to just as U-Boot, is a very popular bootloader for embedded devices. It supports a number of architectures, including ARM, and has pre-made configurations for a very large number of devices, including the Versatile Express series that we're using. Our goal in this article will be to build U-Boot, and combine it with our previously built software. This will not, strictly speaking, change anything significant while we are running on an emulated target in QEMU, but we would need a bootloader in order to run on real hardware.

We will, in this article, change our boot sequence so that U-Boot starts, and then finds our program on a simulated SD card, and subsequently boots it. I will only provide basic explanations for some of the steps, because we'll mostly be dealing with QEMU and Linux specifics here, not really related to ARM programming.

## Preparing U-Boot

First, you should download U-Boot. You could clone the project's source tree, but the easiest way is to download a release [from the official FTP server](ftp://ftp.denx.de/pub/u-boot/). For writing this, I used `u-boot-2018.09`. This is also the reason why your cross-compiler toolchain needs `gcc` of at least version 6 - earlier versions cannot compile U-Boot.

After downloading U-Boot and extracting the sources (or cloning them), you need to run two commands in the U-Boot folder.

```
make vexpress_ca9x4_config ARCH=arm CROSS_COMPILE=arm-none-eabi-
```

This command will prepare some U-Boot configuration, indicating that we want it for the ARM architecture and, more specifically, we want to use the `vexpress_ca9x4` configuration, which corresponds to the `CoreTile Express A9x4` implementation of the Versatile Express platform that we're using.` The configuration command should only take a few seconds to run, after which we can build U-Boot:

```
make all ARCH=arm CROSS_COMPILE=arm-none-eabi-
```

If everything goes well, you should, after a short build process, see the file `u-boot` and `u-boot.bin` created. You can quickly test by running QEMU as usual, except you start U-Boot, providing `-kernel u-boot` on the command line (note that you're booting `u-boot` and not `u-boot.bin`). You should see U-Boot output some information, and you can drop into the U-Boot command mode if you hit a key when prompted.

Having confirmed that you can run U-Boot, make a couple of small modifications to it. In `configs/vexpress_ca9x4_defconfig`, change the `CONFIG_BOOTCOMMAND` line to the following:

```
CONFIG_BOOTCOMMAND="run mmc_elf_bootcmd"
```

The purpose of that will become clear a bit later on. Then open `include/config_distro_bootcmd.h` and go to the end of the file. Find the last line that says `done\0` and edit from there so that the file looks like this:

```
        "done\0"                                          \
    \
    "bootcmd_bare_arm="                                   \
        "mmc dev 0;"                                      \
        "ext2load mmc 0 0x60000000 bare-arm.uimg;"        \
        "bootm 0x60000000;"                               \
        "\0"
```

Note that in the above snippet, the first line with `done\0` was already in the file, but we add a backslash `\` to the end, and then we add the subsequent lines. Regenerate the U-Boot config and rebuild it:

```
make vexpress_ca9x4_config ARCH=arm CROSS_COMPILE=arm-none-eabi-
make all ARCH=arm CROSS_COMPILE=arm-none-eabi-
```

Now would be a good time to start U-Boot in QEMU and verify that everything works. Start QEMU by passing the built U-Boot binary to it in the `-kernel` parameter, like this (where `u-boot-2018.09` is a subfolder name that you might need to change):

```
qemu-system-arm -M vexpress-a9 -m 32M -no-reboot -nographic -monitor telnet:127.0.0.1:1234,server,nowait -kernel u-boot-2018.09/u-boot
```

QEMU should show U-Boot starting up, and if you hit a key when U-Boot prompts `Hit any key to stop autoboot`, you'll be dropped into the U-Boot command line. With that, we can be satisfied that U-Boot was built correctly and works, so next we can tell it to boot something specific, like our program.

## Creating a SD card

On a real hardware board, you would probably have U-Boot and your program stored in the program flash. This doesn't comfortably work with QEMU and the Versatile Express series, so we'll take another approach that is very similar to what you could on hardware. We will create a SD card image, place our program there, and tell U-Boot to boot it. What follows is again not particularly related to ARM programming, but rather a convenient way of preparing an image.

First we'll need an additional package that can be installed with `sudo apt-get install qemu-utils`.

Next we need the SD card image itself, which we can create with `qemu-img`. Then we will create an ext2 partition on the SD card, and finally copy the uImage containing our code to the card (we'll create the uImage in the next section). It is not easily possible to manipulate partitions directly inside an image file, so we will need to mount it using `qemu-nbd`, a tool that makes it possible to mount QEMU images as network block devices. The following script, which I called `create-sd.sh`, can be used to automate the process:

```
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
```

The script creates a 64 megabyte SD card image, mounts it as a network block device, creates a single ext2 partition spanning the entire drive, and copies the supplied uImage to it. From the command line, the script could then be used like

```
./create-sd.sh sdcard.img bare-arm.uimg
```

to create an image called `sdcard.img` and copy the `bare-arm.uimg` uImage onto the emulated SD card.

---

**NOTE**

Depending on your system, you might get an error about `/dev/nbd0` being unavailable when you run the SD card creation script. The most likely cause of such an error is that you don't have the `nbd` kernel module loaded. Loading it with `sudo modprobe nbd` should create `/dev/nbd0`. To permanently add the module to the load list, you can do `echo "nbd" | sudo tee -a /etc/modules`

---

## Creating the uImage

Now that we have created a SD card and can copy an uImage to it, we have to create the uImage itself.

First of all, what is an uImage? The U-Boot bootloader can load applications from different types of images. These images can consist of multiple parts, and be fairly complex, like how Linux gets booted. We are not trying to boot a Linux kernel or anything else complicated, so we'll be using an older image format for U-Boot, which is then the uImage format. The uImage format consists of simply the raw data and a header that describes the image. Such images can be created with the `mkimage` utility, which is part of U-Boot itself. When we built U-Boot, `mkimage` should have been built as well.

Let's call `mkimage` and ask it to create an U-Boot uImage out of the application we had previously, the "better hang" one. From now on, we'll also be able to use ELF files instead of the raw binary dumps because U-Boot knows how to load ELF files. `mkimage` should be located in the `tools` subfolder of the U-Boot folder. Assuming our `better-hang.elf` is still present, we can do the following:

```
u-boot-2018.09/tools/mkimage -A arm -C none -T kernel -a 0x60000000 -e 0x60000000 -d better-hang.elf bare-arm.uimg
```

With that, we say that we want an uncompressed (`-C none`) image for ARM (`-A arm`), the image will contain an OS kernel (`-T kernel`). With `-d better-hang.bin` we tell `mkimage` to put that `.bin` file into the image. We told U-Boot that our image will be a kernel, which is not really true because we don't have an operating system. But the `kernel` image type indicates to U-Boot that the application is not going to return control to U-Boot, and that it will manage interrupts and other low-level things by itself. This is what we want since we're looking at how to do low-level programming in bare metal.

We also indicate that the image should be loaded at `0x60000000` (with `-a`) and that the entry point for the code will be at the same address (with `-e`). This choice of address is because we want to load the image into the RAM of our device, and in the previous chapter we found that RAM starts at `0x60000000` on the board. Is it safe to place our code into the beginning of RAM? Will it not overwrite U-Boot itself and prevent a proper boot? Fortunately, we don't have that complication. U-Boot is initially executed from ROM, and then, on ARM system, it copies itself to the **end** of the RAM before continuing from there.

When the uImage is created, we need to copy it to the SD card. As noted previously, it can be done just by executing `./create-sd.sh sdcard.img bare-arm.uimg` thanks to the script created before. If everything went well, we now have a SD card image that can be supplied to QEMU.

## Booting everything

We're ready to boot! Let's start QEMU as usual, except that this time we'll also add an extra parameter telling QEMU that we want to use a SD card.

```
qemu-system-arm -M vexpress-a9 -m 32M -no-reboot -nographic -monitor telnet:127.0.0.1:1234,server,nowait -kernel u-boot-2018.09/u-boot -sd sdcard.img
```

Hit a key when U-Boot prompts you to, in order to use the U-Boot command line interface. We can now use a few commands to examine the state of things and confirm that everything is as we wanted. First type `mmc list` and you should get a response like `MMC: 0`. This confirms the presence of an emulated SD card. Then type `ext2ls mmc 0`. That is the equivalent of running `ls` on the SD card's filesystem, and you should see a response that includes the `bare-arm.uimg` file - our uImage.

Let's load the uImage into memory. We can tell U-Boot to do that with `ext2load mmc 0 0x60000000 bare-arm.uimg`, which as can probably guess means to load `bare-arm.uimg` from the `ext2` system on the first MMC device into address `0x60000000`. U-Boot should report success, and then we can use `iminfo 0x60000000` to verify whether the image is located at that address now. If everything went well, U-Boot should report that a legacy ARM image has been found at the address, along with a bit more information about the image. Now we can go and boot the image from memory: `bootm 0x60000000`.

U-Boot will print `Starting kernel...` and seemingly hang. You can now check the QEMU monitor (recall that you can connect to it with `telnet localhost 1234`), and issue the `info registers` command to see that R2 is once again equal to `0xDEADBEEF`. Success! Our program has now been loaded from a SD card image and started through U-Boot!

Better yet, the modifications we made earlier to U-Boot allow it to perform this boot sequence automatically. With those modifications, we added a new environment variable to U-Boot, `bootcmd_bare_arm`, which contains the boot commands. If you type `printenv bootcmd_bare_arm` in the U-Boot command-line, you'll see the boot sequence.

If you start QEMU again and don't press any keys to pause U-Boot, you should see  If you type `printenv bootcmd_bare_arm` in the U-Boot command-line, you'll see the boot sequence. If you start QEMU again and don't press any keys to pause U-Boot, you should see the boot continue automatically.

Having now completed a boot sequence fairly similar to real hardware, we can continue with our own programming. In the next part, we'll continue by getting some C code running.
