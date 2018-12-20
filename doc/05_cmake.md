# Build & debug system

This part is going to be a detour from bare-metal programming in order to quickly set up a build system using CMake, and briefly show how our program can be debugged while running in QEMU. If you're not interested, you can skip this part, though at least skimming it would be recommended.

We will use [CMake](https://cmake.org) as our build manager. CMake provides a powerful language to define projects, and doesn't actually build them itself - CMake generates input for another build system, which will be GNU Make since we're developing on Linux. It's also possible to use Make by itself, but for new projects I prefer to use CMake even when its cross-platform capabilities and other powerful features are not needed.

It should also be noted here that I am far from a CMake expert, and build systems aren't the focus of these articles, so this could certainly be done better.

To begin with, we'll organize our project folder with some subfolders, and create a top-level `CMakeLists.txt`, which is the input file CMake normally expects. The better-organized project structure looks like this:

```
|-- CMakeLists.txt
|-- scripts
|   `-- create-sd.sh
`-- src
    |-- cstart.c
    |-- linkscript.ld
    `-- startup.s
```

The `src` folder contains all the source code, the `scripts` folder is for utility scripts like the one creating our SD card image, and at the top there's `CMakeLists.txt`.

We want CMake to handle the following for us:

* Rebuild U-Boot if necessary
* Build our program, including the binary converstion with `objcopy`
* Create the SD card image for QEMU
* Provide a way of running QEMU

To accomplish that, `CMakeLists.txt` can contain the following:

```
cmake_minimum_required (VERSION 2.8)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_CROSSCOMPILING TRUE)

set(UBOOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../common_uboot")
set(MKIMAGE "${UBOOT_PATH}/tools/mkimage")

project (bare-metal-arm C ASM)

set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_ASM_COMPILER "arm-none-eabi-as")
set(CMAKE_OBJCOPY "arm-none-eabi-objcopy")

file(GLOB LINKSCRIPT "src/linkscript.ld")
set(ASMFILES src/startup.s)
set(SRCLIST src/cstart.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostartfiles -nostdlib -g -Wall")
set(CMAKE_EXE_LINKER_FLAGS "-T ${LINKSCRIPT}")

add_custom_target(u-boot 
            COMMAND make vexpress_ca9x4_config ARCH=arm CROSS_COMPILE=arm-none-eabi- 
            COMMAND make all ARCH=arm CROSS_COMPILE=arm-none-eabi- 
            WORKING_DIRECTORY ${UBOOT_PATH})

add_executable(bare-metal ${SRCLIST} ${ASMFILES})
set_target_properties(bare-metal PROPERTIES OUTPUT_NAME "bare-metal.elf")
add_dependencies(bare-metal u-boot)

add_custom_command(TARGET bare-metal POST_BUILD COMMAND ${CMAKE_OBJCOPY}
    -O binary bare-metal.elf bare-metal.bin COMMENT "Converting ELF to binary")

add_custom_command(TARGET bare-metal POST_BUILD COMMAND ${MKIMAGE}
    -A arm -C none -T kernel -a 0x60000000 -e 0x60000000 -d bare-metal.bin bare-arm.uimg
    COMMENT "Building U-Boot image")

add_custom_command(TARGET bare-metal POST_BUILD COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/create-sd.sh 
    sdcard.img bare-arm.uimg
    COMMENT "Creating SD card image")

add_custom_target(run)
add_custom_command(TARGET run POST_BUILD COMMAND 
                 qemu-system-arm -M vexpress-a9 -m 512M -no-reboot -nographic 
                 -monitor telnet:127.0.0.1:1234,server,nowait -kernel ${UBOOT_PATH}/u-boot -sd sdcard.img -serial mon:stdio
                 COMMENT "Running QEMU...")
```

We begin with some preparation, telling CMake that we'll be cross-compiling so it doesn't assume it's building a Linux application, and we store the U-Boot path in a variable. Then there's the specification of the project itself:

```
project (bare-metal-arm C ASM)
```

`bare-metal-arm` is an arbitrary project name, and we indicate that C and assembly files are used in it. The next few lines explicitly specify the cross-compiler toolchain elements to be used. After that, we list the source files included in the project:

```
file(GLOB LINKSCRIPT "src/linkscript.ld")
set(ASMFILES src/startup.s)
set(SRCLIST src/cstart.c)
```

Some people prefer to specify wildcards so that all `.c` files are included automatically, but CMake guidelines recommend against this approach. This is a matter of debate, but here I chose to go with explicitly listed files, meaning that new files will need to manually be added here.

CMake lets us specify flags to be passed to the compiler and linker, which we do:

```
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostartfiles -nostdlib -g -Wall")
set(CMAKE_EXE_LINKER_FLAGS "-T ${LINKSCRIPT}")
```

Those are like what we used previously, except for the addition of `-g -Wall`. The `-g` switch enables generation of debug symbols, which we'll need soon, and `-Wall` enables all compiler warnings and is very good to use as a matter of general practice.

We then define a custom target to build U-Boot, which just runs U-Boot's regular make commands. After that we're ready to define our main target, which we say is an executable that should be built out of source files in the `SRCLIST` and `ASMFILES` variables, and should be called `bare-metal.elf`:

```
add_executable(bare-metal ${SRCLIST} ${ASMFILES})
set_target_properties(bare-metal PROPERTIES OUTPUT_NAME "bare-metal.elf")
```

The two subsequent uses of `add_custom_command` are to invoke `objcopy` and call the `create-sd.sh` script to build the U-Boot uimage.

That's everything we need to build our entire program, all the way to the U-Boot image containing it. It is, however, also convenient to have a way of running QEMU from the same environment, so we also define a target called `run` and provide the QEMU command-line to it. The addition of `-serial mon:stdio` in the QEMU command line means that we'll be able to issue certain QEMU monitor commands directly in the terminal, giving us a cleaner shutdown option.

## Building and running

When building the project, I strongly recommend doing what's known as an *out of source build*. It simply means that all the files resulting from the build should go into a separate folder, and not your source folder. This is cleaner (no mixing of source and object files), easier to use with Git (just ignore the whole build folder), and allows you to have several builds at the same time.

To build with CMake, first you need to tell CMake to generate the build configuration from `CMakeLists.txt`. The easiest way to perform a build is:

```
cmake -S . -Bbuild
```

The `-S .` option says to look for the source, starting with `CMakeLists.txt`, in the current folder, and `-Bbuild` specifies that a folder called `build` should contain the generated configuration. After running that command, you'll have the `build` folder containing configurations generated by CMake. You can then use `make` inside that folder. There's no need to call CMake itself again unless the build configuration is supposed to change. If you, for instance, add a new source file to the project, you need to include it in `CMakeLists.txt` and call CMake again.

From the newly created `build` folder, simply invoking the default make target will build everything.

```
make
```

You'll see compilation of U-Boot and our own project, and the other steps culminating in the creation of `sdcard.img`. Since we also defined a CMake target to run QEMU, it can also be invoked directly after building. Just do:

```
make run
```

and QEMU will run our program. Far more convenent than what we had been doing.

---

**HINT**

If you run QEMU with `make run` and then terminate it with Ctrl-C, you'll get messages about the target having failed. This is harmless but doesn't look nice. Instead, you can cleanly quit QEMU with Ctrl-A, X (that is Ctrl-A first and then X without the Ctrl key). It's a feature of the QEMU monitor, and works because of adding `-serial mon:stdio` to the QEMU command-line.

---

## Debugging in QEMU with GDB

While the QEMU monitor provides many useful features, it's not a proper debugger. When running software on a PC through QEMU, as opposed to running on real hardware, it would be a waste not to take advantage of the superior debug capabilities available. We can debug our bare-metal program using the GDB, the GNU debugger. GDB provides remote debugging capabilities, with a server called `gdbserver` running on the machine to be debugged, and then the main `gdb` client communicatng with the server. QEMU is able to start an instance of `gdbserver` along with the program it's emulating, so remote debugging is a possibility with QEMU and GDB.

Starting `gdbserver` when running QEMU is as easy as adding `-gdb tcp::2159` to the QEMU command line (2159 is the standard port for GDB remote debugging). Given that we're using CMake, we can use it to define a new target for a debug run of QEMU. These are the additions in `CMakeLists.txt`:


```
string(CONCAT GDBSCRIPT "target remote localhost:2159\n"
                        "file bare-metal.elf")
file(WRITE ${CMAKE_BINARY_DIR}/gdbscript ${GDBSCRIPT})

add_custom_target(drun)
add_custom_command(TARGET drun PRE_BUILD COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
                    "To connect the debugger, run arm-none-eabi-gdb -x gdbscript")
add_custom_command(TARGET drun PRE_BUILD COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
                    "To start execution, type continue in gdb")

add_custom_command(TARGET drun POST_BUILD COMMAND 
                 qemu-system-arm -S -M vexpress-a9 -m 512M -no-reboot -nographic -gdb tcp::2159
                 -monitor telnet:127.0.0.1:1234,server,nowait -kernel ${UBOOT_PATH}/u-boot -sd sdcard.img -serial mon:stdio
                 COMMENT "Running QEMU with debug server...")
```

The `drun` target (for *debug run*) adds `-gdb tcp::2159` to start `gdbserver`, and `-S`, which tells QEMU not to start execution after loading. That option is useful for debugging because it gives you the time to set breakpoints, letting you debug the code very early if you need to.

When debugging remotely, GDB needs to know what server to connect to, and where to get the debug symbols. We can connect using the GDB command `target remote localost:2159` and then load the ELF file using `file bare-metal.elf`. To avoid typing those commands manually all the time, we ask CMake to put them into a file called `gdbscript` that GDB can read when started.

Let's rebuild and try a quick debug session.

```
cmake -S . -Bbuild
cd build
make
make drun
```

You should see CMake print some hints that we provided, and then QEMU will wait doing nothing. In another terminal now, you can start GDB from the `build` folder, telling it to read commands from the `gdbscript` file:

```
arm-none-eabi-gdb -x gdbscript
```

Now you have GDB running and displaying `(gdb)`, its command prompt. You can set a breakpoint using the `break` command, let's try to set one in the `main` function, and then continue execution with `c` (short for `continue`):

```
(gdb) break main
(gdb) c
```

As soon as you issue the `c` command, you'll see QEMU running. After U-Boot output, it will stop again, and GDB will show that it hit a breakpoint, something like the following:

```
Breakpoint 1, main () at /some/path/to/repo/src/05_cmake/src/cstart.c:13
13              const char* s = "Hello world from bare-metal!\n";

```

From there, you can use the `n` command to step through the source code, `info stack` to see the stack trace, and any other GDB commands.

I won't be covering GDB in additional detail here, that's outside the scope of these tutorials. GDB has a comprehensive, if overwhelming, manual, and there's a lot more material available online. [Beej's guide to GDB](https://beej.us/guide/bggdb/), authored by Brian Hall, is perhaps the best getting-started guide for GDB. If you'd rather use a graphical front-end, there is also a large selection. When looking for GDB-related information online, don't be alarmed if you find old articles - GDB dates back to the 1980s, and while it keeps getting new features, the basics haven't changed in a very long time.

Our project now has a half-decent build system, we are no longer relying on manual steps, and can debug our program. This is a good place to continue from!
