# The first boot

We'll continue our introduction to bare-metal ARM by starting an emulated ARM machine in QEMU, and using the cross-compiler toolchain to load the simplest possible code into it.

Let us run QEMU for the very first time, with the following command:

```
qemu-system-arm -M vexpress-a9 -m 32M -no-reboot -nographic -monitor telnet:127.0.0.1:1234,server,nowait
```

It should take some time as the QEMU machine spins up, after which it should crash with an error message. The crash is to be expected - we did not provide any executable to run so of course our emulated system cannot accomplish anything. For documenation of the QEMU command line, you can check `man qemu-doc` and online, but let's go through the command we used and break it down into parts. 

* `-M vexpress-a9`. The `-M` switch selects the specific machine to be emulated. The [ARM Versatile Express](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.subset.boards.express/index.html) is an ARM platform intended for prototyping and testing. Hardware boards with the Versatile Express platform exist, and the platform is also a common choice for testing with emulation. The `vexpress-a9` variant has the Cortex A9 CPU, which powers a wide variety of embedded devices that perform computationally-intensive tasks.

* `-m 32M`. This simply sets the RAM of the emulated machine to 32 megabytes.

* `-no-reboot`. Don't reboot the system if it crashes.

* `-nographic`. Run QEMU as a command-line application, outputting everything to a terminal console. The serial port is also redirected to the terminal.

* `-monitor telnet:127.0.0.1:1234,server,nowait`. One of the advantages of QEMU is that it comes with a powerful *QEMU monitor*, an interface to examine the emulated machine and control it. Here we say that the monitor should run on localhost, port `1234`, with the `server,nowait` options meaning that QEMU will provide a telnet server but will continue running even if nobody connects to it.

That's it for the first step - now you have a command to start an ARM system, although it will not do anything except crashing with a message like `qemu-system-arm: Trying to execute code outside RAM or ROM at 0x04000000`.

# The first hang

## Writing some code

Now we want to write some code, turn it into an executable that can run on an ARM system, and run it in QEMU. This will also serve as our first cross-compilation attempt, so let's go with the simplest code possible - we will write a value to one of the CPU registers, and then enter an infinite loop, letting our (emulated) hardware hang indefinitely. Since we're doing bare-metal programming and have no form of runtime or operating system, we have to write the code in assembly language. Create a file called `startup.s` and write the following code:

```
ldr r2,str1
b .
str1: .word 0xDEADBEEF
```

Line by line:

1. We load the value at label `str1` (which we will define shortly) into the register `R2`, which is one of the general-purpose registers in the ARM architecture.

2. We enter an infinite loop. The period `.` is short-hand for *current address*, so `b .` means "branch to the current instruction address", which is an infinite loop.

3. We allocate a word (4 bytes) with the value `0xDEADBEEF`, and give it the label `str1`. The value `0xDEADBEEF` is a distinctive value that we should easily notice. Writing such values is a common trick in low-level debugging, and `0xDEADBEEF` is often used to indicate free memory or a general software crash. Why will this work if we're in an infinite loop? Because this is not executable code, it's just an instruction to the assembler to allocate the 4-byte word here.

## Assembling it

Next we need to compile the code. Since we only wrote assembly code, compilation is not actually relevant, so we will just assemble and link the code. The first time, let's do this manually to see how to use the cross-compiler toolchain correctly. What we're doing here is very much not optimal even for an example as simple as this, and we'll improve the state of things soon.

First, we need to assemble `startup.s`, which we can do like this, telling the GNU Assembler (`as`) to place the output in `startup.o`.

```
arm-none-eabi-as -o startup.o startup.s
```

We do not yet have any C files, so we can go ahead and link the object file, obtaining an executable.

```
arm-none-eabi-ld -o first-hang.elf startup.o
```

This will create the executable file `first-hang.elf`. You will also see a warning about missing `_start`. The linker expects your code to include a `_start` symbol, which is normally where the execution would start from. We can ignore this now because we only need the ELF file as an intermediate step anyway. The `first-hang.elf` which you obtained is a sizable executable, reaching 33 kilobytes on my system. ELF executables are the standard for Linux and other Unix-like systems, but they need to be loaded and executed by an operating system, which we do not have. An ELF executable is therefore not something we can use on bare metal, so the next step is to convert the ELF to a raw binary dump of the code, like this:

```
arm-none-eabi-objcopy -O binary first-hang.elf first-hang.bin
```

The resulting `first-hang.bin` is just a 12-byte file, that's all the space necessary for the code we wrote in `startup.s`. If we look at the hexdump of this file, we'll see `00000000  00 20 9f e5 fe ff ff ea  ef be ad de`. You can recognize our `0xDEADBEEF` constant at the end. The first eight bytes are our assembly instructions in raw form. The code starts with `0020 9fe5`. The `ldr` instruction has the opcode `e5`, then `9f` is a reference to the program counter (PC) register, and the `20` refers to `R2`, meaning this is in fact `ldr r2, [pc]` encoded.

Looking at the hexdump of a binary and trying to match the bytes to assembly instructions is uncommon even for low-level programming. It is somewhat useful here as an illustration, to see how we can go from writing `startup.s` to code executable by an ARM CPU, but this is more detail than you would typically need.

## And... Blastoff!

We can finally run our code on the ARM machine! Let's do so.

```
qemu-system-arm -M vexpress-a9 -m 32M -no-reboot -nographic -monitor telnet:127.0.0.1:1234,server,nowait -kernel first-hang.bin
```

This runs QEMU like previously, but we also pass `-kernel first-hang.bin`, indicating that we want to load our binary file into the emulated machine. This time you should see QEMU hang indefinitely. The QEMU monitor allows us to read the emulated machine's registers, among other things, so we can check whether our code has actually executed. Open a telnet connection in another terminal with `telnet localhost 1234`, which should drop you into the QEMU monitor's command line, looking something like this:

```
QEMU 2.8.1 monitor - type 'help' for more information
(qemu) 
```

At the `(qemu)` prompt, type `info registers`. That's the monitor command to view registers. Near the beginning of the output, you should spot our `0xDEADBEEF` constant that has been loaded into R2:

```
R00=00000000 R01=000008e0 R02=deadbeef R03=00000000
```

This means that yes indeed, QEMU has successfully executed the code we wrote. Not at all fancy, but it worked. We have our first register write and hang.

# What we did wrong

Our code worked, but even in this small example we didn't really do things the right way.

## Memory mappings

One issue is that we didn't explicitly specify any start symbol that would show where our program should begin executing. It works because when the CPU starts up, it begins executing from address `0x0`, and we have placed a valid instruction at that address. But it could easily go wrong. Consider this variation of `startup.s`, where we move the third line to the beginning.

```
str1: .word 0xDEADBEEF
ldr r2,str1
b .
```

That is still valid assembly and feels like it should work, but it wouldn't - the constant `0xDEADBEEF` would end up at address `0x0`, and that's not a valid instruction to begin the program with. Moreover, even starting at address `0x0` isn't really correct. On a real system, the interrupt vector table should be located at address `0x0`, and the general boot process should first have the bootloader starting, and after that switch execution to your code, which is loaded somewhere else in memory.

QEMU is primarily used for running Linux or other Unix-like kernels, which is reflected in how it's normally started. When we start QEMU with `-kernel first-hang.bin`, QEMU acts as if booting such a kernel. It copies our code to the memory location `0x10000`, that is, a 64 kilobyte offset from the beginning of RAM. Then it starts executing from the address `0x0`, where QEMU already has some startup code meant to prepare the machine and jump to the kernel. 

Sounds like we should be able to find our `first-hang.bin` at `0x10000` in the QEMU memory then. Let's try do to that in the QEMU monitor, using the `xp` command which displays physical memory. In the QEMU monitor prompt, type `xp /4w 0x100000` to display the four words starting with that memory address.

```
0000000000100000: 0x00000000 0x00000000 0x00000000 0x00000000
```

Everything is zero! If you check the address `0x0`, you will find the same. How come?

The answer is memory mapping - the address space of the device encompasses more than just the RAM. It's time to consult the most important document when developing for a particular device, its *Technical Reference Manual*, or TRM for short. The TRM for any embedded device is likely to have a section called "memory map" or something along those lines. The TRM for our device is [available from ARM](https://developer.arm.com/products/system-design/development-boards/soft-macro-models/docs/dui0448/latest/preface), and it indeed [contains a memory map](https://developer.arm.com/products/system-design/development-boards/soft-macro-models/docs/dui0448/latest/programmers-model/daughterboard-memory-map). (Note: when working with any device, downloading a PDF version of the TRM is a very good idea.) In this memory map, we can see that the device's RAM (denoted as "local DDR2") begins at `0x60000000`.

That means we have to add `0x60000000` to RAM addresses to obtain the physical address, so our `0x10000` where we expect the binary code to be loaded is at physical address `0x60010000`. Let's check if we can find the code at that address: `xp /4w 0x60010000` shows us:

```
0000000060010000: 0xe59f2000 0xeafffffe 0xdeadbeef 0x00000000
```

There it is indeed, our `first-hang.bin` loaded into memory! 

## Creating the vector table

Having our code start at address `0x0` isn't acceptable as explained before, as that is the address where the interrupt vector table is expected. We should also not rely on things just working out, with help from QEMU or without, and should explicitly specify the entry point for our program. Finally, we should separate code and data, placing them in separate sections. Let's start by improving our `startup.s` a bit:

```
.section .vector_table, "x"
.global _Reset
_Reset:
    b Reset_Handler
    b . /* 0x4  Undefined Instruction */
    b . /* 0x8  Software Interrupt */
    b . /* 0xC  Prefetch Abort */
    b . /* 0x10 Data Abort */
    b . /* 0x14 Reserved */
    b . /* 0x18 IRQ */
    b . /* 0x1C FIQ */

.section .text
Reset_Handler:
    ldr r2, str1
    b .
    str1: .word 0xDEADBEEF
```

Here are the things we're doing differently this time:

1. We're creating the vector table at address `0x0`, putting it in a separate section called `.vector_table`, and declaring the global symbol `_Reset` to point to its beginning. We leave most items in the vector table undefined, except for the reset vector, where we place the instruction `b Reset_Handler`.

2. We moved our executable code to the `.text` section, which is the standard section for code. The `Reset_Handler` label points to the code so that the reset interrupt vector will jump to it.

## Creating the linker script

Linker scripts are a key component in building embedded software. They provide the linker with information on how to place the various sections in memory, among other things. Let's create a linker script for our simple program, call it `linkscript.ld`.

```
ENTRY(_Reset)

SECTIONS
{
    . = 0x0;
    .text : { startup.o (.vector_table) *(.text) } 
    . = ALIGN(8);
}
```

This script tells the linker that the program's entry point is at the global symbol `_Entry`, which we export from `startup.s`. Then the script goes on to list the section layout. Starting at address `0x0`, we create the `.text` section for code, consisting first of the `.vector_table` section from `startup.o`, and then any and all other `.text` sections. We align the code to an 8-byte boundary as well.

# Hanging again - but better

We can now build the updated software. Let's do that in a similar manner to before:

```
arm-none-eabi-as -o startup.o startup.s
arm-none-eabi-ld -T linkscript.ld -o better-hang.elf startup.o
arm-none-eabi-objcopy -O binary better-hang.elf better-hang.bin
```

Note the addition of `-T linkscript.ld` to the linker command, specifying to use our newly created linker script. We still cannot use the ELF file directly, but we could use `objdump` to verify that our linkscript changed things. Call `arm-none-eabi-objdump -h better-hang.elf` to see the list of sections. You'll notice the `.text` section. And if you use `objdump` to view `startup.o`, you'll also see `.vector_table`. You can even observe that the sizes of `.vector_table` and `.text` in `startup.o` add up to the size of `.text` in the ELF file, further indicating that things are probably as we wanted.

We can now once again run the software in QEMU with `qemu-system-arm -M vexpress-a9 -m 32M -no-reboot -nographic -monitor telnet:127.0.0.1:1234,server,nowait -kernel better-hang.bin` and observe the same results as before, and happily knowing things are now done in a more proper way.

In the next part, we will continue by introducing a bootloader into our experiments.
