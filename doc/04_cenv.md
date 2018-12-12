# Preparing a C environment

In this part, we will do some significant work to get a proper application up and running. What we have done so far is hardly an application, we only execute one instruction before hanging, and everything is done in the reset handler. Also, we haven't written or run any C code so far. Programming in assembly is harder than in C, so generally bare-metal programs will do a small amount of initialization in assembly and then hand control over to C code as soon as possible.

We are now going to write startup code that prepares a C environment and runs the `main` function in C. This will require setting up the stack and also handling data relocation, that is, copying some data from ROM to RAM. The first C code that we run will print some strings to the terminal by accessing the UART peripheral device. This isn't really using a proper driver, but performing some UART prints is a very good way of seeing that things are working properly.

# New startup code

## Setting up the stack

Our startup code is getting a major rework. It will do several new and exciting things, the most basic of which is to prepare the stack. C code will not execute properly without a stack, which is necessary among other things to have working function calls.

Conceptually, preparing the stack is quite simple. We just pick two addresses in our memory space that will be the start and end of the stack, and then set the initial stack pointer to the start of the stack. By convention, the stack grows towards lower addresses, that is, your stack could be between addresses like `0x60020000` and `0x60021000`, which would give a stack of `0x1000` bytes, and the stack pointer would initially point to the "end" address `0x600210000`.

The ARMv7A architecture has, on a system level, several stack pointers and the CPU has several processor modes. Consulting the ARMv7A reference manual, we can see that processor modes are: user, FIQ, IRQ, supervisor, monitor, abort, undefined and system. To simplify things, we will only care about three modes now - the FIQ and IRQ modes, which execute fast interrupt and normal interrupt code respectively - and the supervisor mode, which is the default mode the processor starts in.

Further consulting the manual (section *B1.3.2 ARM core registers*), we see that the ARM core registers, R0 to R15, differ somewhat depending on the mode. We are now interested in the stack pointer register, SP or R13, and the manual indicates that the "current" SP register an application sees depends on the processor mode, with the supervisor, IRQ and FIQ modes each having their own SP register.

---
**NOTE**

Referring to ARM registers, SP and R13 always means the same thing. Similarly, LR is R14 and PC is R15. The name used depends on the documentation or the tool you're using, but don't get confused by the three special registers R13-R15 having multiple names.

---

In our startup code, we are going to switch to all the relevant modes in order and set the initial stack pointer in the SP register to a memory address that we allocate for the purpose. We'll also fill the stack with a garbage pattern to indicate it's unused.

First we add some defines at the beginning of our `startup.s`, with values for the different modes taken from the ARMv7A manual.

```
/* Some defines */
.equ MODE_FIQ, 0x11
.equ MODE_IRQ, 0x12
.equ MODE_SVC, 0x13
```

Now we change our entry code in `Reset_Handler` so that it starts like this:

```
Reset_Handler:
    /* FIQ stack */
    msr cpsr_c, MODE_FIQ
    ldr r1, =_fiq_stack_start
    ldr sp, =_fiq_stack_end
    movw r0, #0xFEFE
    movt r0, #0xFEFE

fiq_loop:
    cmp r1, sp
    strlt r0, [r1], #4
    blt fiq_loop
```

Let's walk through the code in more detail.

```
    msr cpsr_c, MODE_FIQ
```

One of the most important registers in an ARMv7 CPU is the `CPSR` register, which stands for *Current Program Status Register*. This register can be written with the special `msr` instruction - a regular `mov` will not work. `cpsr_c` is used in the instruction in order to change `CPSR` without affecting the condition flags in bits 28-31 of the `CPSR` value. The least significant five bits of `CPSR` form the mode field, so writing to it is the way to switch processor modes. By writing `0x11` to the `CPSR` mode field, we switch the processor to FIQ mode.

```
    ldr r1, =_fiq_stack_start
    ldr sp, =_fiq_stack_end
```

We load the end address of the FIQ stack into `SP`, and the start address into `R1`. Just setting the `SP` register would be sufficient, but we'll use the start address in `R1` in order to fill the stack. The actual addresses for `_fiq_stack_end` and other symbols we're using in stack initialization will be output by the linker with the help of our linkscript, covered later.

```
    movw r0, #0xFEFE
    movt r0, #0xFEFE
```

These two lines just write the value `0xFEFEFEFE` to `R0`. ARM assembly has limitations on what values can be directly loaded into a register with the `mov` instruction, so one common way to load a 4-byte constant into a register is to use `movw` and `movt` together. In general `movw r0, x; movw r0, y` corresponds to loading `x << 16 | y` into `R0`.

```
fiq_loop:
    cmp r1, sp
    strlt r0, [r1], #4
    blt fiq_loop
```

---
**NOTE**

The `strlt` and `blt` assembly instructions you see above are examples of ARM's *conditional execution* instructions. Many instructions have conditional forms, in which the instruction has a condition code suffix. These instructions are only executed if certain flags are set. The store instruction is normally `str`, and then `strlt` is the conditional form that will only execute if the `lt` condition code is met, standing for less-than. So a simplified way to state things is that `strlt` will only execute if the previous compare instruction resulted in a less-than status.

---

This is the loop that actually fills the stack with `0xFEFEFEFE`. First it compares the value in R1 to the value in SP. If R1 is less than SP, the value in R0 will be written to the address stored in R1, and R1 gets increased by 4. Then the loop continues as long as R1 is less than SP.

Once the loop is over and the FIQ stack is ready, we repeat the process with the IRQ stack, and finally the supervisor mode stack (see the code in the full listing of `startup.s` at the end).

## Handling sections and data

### A rundown on sections

To get the next steps right, we have to understand the main segments that a program normally contains, and how they normally appear in ELF file sections.

* The code segment, normally called `.text`. It contains the program's executable code, which normally means it's read-only and has a known size.

* The data segment, normally called `.data`. It contains data that can be modified by the program at runtime. In C terms, global and static variables that have a non-zero initial value will normally go here.

* The read-only data segment, usually with a corresponding section called `.rodata`, sometimes merged with `.text`. Contains data that cannot be modified at runtime, in C terms constants will be stored here.

* The unitialized data segment, also known as the BSS segment and with the corresponding section being `.bss`. Don't worry about the strange name, it has a history dating back to the 1950s. C variables that are static and have no initial value will end up here. The BSS segment is expected to be filled with zeroes, so variables here will end up initialized to zero. Modern compilers are also smart enough to assign variables explicitly initialized with zero to BSS, that is, `static int x = 0;` would end up in `.bss`.

Thinking now about our program being stored in some kind of read-only memory, like the onboard flash memory of a device, we can reason about which sections have to be copied into the RAM of the device. There's no need to copy `.text` - code can be executed directly from ROM. Same for `.rodata`, constants can be read from the ROM. However, `.data` is for modifiable variables and therefore needs to be in RAM. `.bss` needs to be created in RAM and zeroed out.

Most embedded programs need to take care of ROM-to-RAM data copying. The specifics depend on the hardware and the use case. On larger, more capable single-board computers, like the popular Raspberry Pi series, it's reasonable to copy even the code (`.text` section) to RAM and run from RAM because of the performance benefits as opposed to reading the ROM. On microcontrollers, copying the code to RAM isn't even an option much of the time, as a typical ARM-based microcontroller using a Cortex-M3 CPU or similar might have around 1 megabyte flash memory but only 96 kilobytes of RAM.

### New section layout

Previously we had written `linkscript.ld`, a small linker script. Now we will need a more complete linker script that will define the data sections as well. It will also need to export the start and end addresses of sections so that copying code can use them, and finally, the linker script will also export some symbols for the stack, like the `_fiq_stack_start` that we used in our stack setup code.

Normally, we would expect the program to be stored in flash or other form of ROM, as mentioned before. With QEMU it is possible to emulate flash memory on a number of platforms, but unfortunately not on the Versatile Express series. We'll do something different then and pretend that a section of the emulated RAM is actually ROM. Let's say that the area at `0x60000000`, where RAM actually starts, is going to be treated as ROM. And let's then say that we'll use `0x70000000` as the pretend starting point of RAM. There's no need to skip so much memory - the two points are 256 MB apart - but it's then very easy to look at addresses and immediately know if it's RAM from the first digit.

The first thing to do in the linker script, then, is to define the two different memory areas, our (pretend) ROM and RAM. We do this after the `ENTRY` directive, using a `MEMORY` block.

```
MEMORY
{
    ROM (rx) : ORIGIN = 0x60000000, LENGTH = 1M
    RAM (rwx): ORIGIN = 0x70000000, LENGTH = 32M
}
```

---
**NOTE**

The GNU linker, `ld`, can occasionally appear to be picky with the syntax. In the snippet above, the spaces are also significant. If you write `LENGTH=1M` without spaces, it won't work, and you'll be rewarded with a terse "syntax error" message from the linker.

---

Our section definitions will now be like this:

```
    .text : {
        startup.o (.vector_table)
        *(.text)
        *(.rodata)
     } > ROM
    _text_end = .;
    .data : AT(ADDR(.text) + SIZEOF(.text))
    {
        _data_start = .;
        *(.data)
        . = ALIGN(8);
        _data_end = .;
    } > RAM
    .bss : {
        _bss_start = .;
        *(.bss)
        . = ALIGN(8);
        _bss_end = .;
    } > RAM
```

The `.text` section is similar to what we had before, but we're also going to append `.rodata` to it to make life easier with one section less. We write `> ROM` to indicate that `.text` should be linked to ROM. The `_text_end` symbol exports the address where `.text` ends in ROM and hence where the next section starts.

`.data` follows next, and we use `AT` to specify the load address as being right after `.text`. We collect all input `.data` sections in our output `.data` section and link it all to RAM, defining `_data_start` and `_data_end` as the RAM addresses where the `.data` section will reside at runtime. These two symbols are written inside the block that ends with `> RAM`, hence they will be RAM addresses.

`.bss` is handled in a similar manner, linking it to RAM and exporting the start and end addresses.

### Copying ROM to RAM

As discussed, we need to copy the `.data` section from ROM to RAM in our startup code. Thanks to the linker script, we know where `.data` starts in ROM, and where it should start and end in RAM, which is all the information we need to perform the copying. In `startup.s`, after dealing with the stacks we now have the following code:

```
    /* Start copying data */
    ldr r0, =_text_end
    ldr r1, =_data_start
    ldr r2, =_data_end

data_loop:
    cmp r1, r2
    ldrlt r3, [r0], #4
    strlt r3, [r1], #4
    blt data_loop
```

We begin by preparing some data in registers. We load `_text_end` into `R0`, with `_text_end` being the address in ROM where `.text` has ended and `.data` starts. `_data_start` is the address in RAM at which `.data` should start, and `_data_end` is correspondingly the end address in RAM. Then the loop itself compares `R1` and `R2` registers and, as long as `R1` is smaller (meaning we haven't reached `_data_end`), we first load 4 bytes of data from ROM into `R3`, and then store these bytes at the memory address in `R1`. The `#4` operand in the load and store instructions ensures we're increasing the values in `R0` and `R1` correspondingly so that loop continues over the entirety of `.data` in ROM.

With that done, we also initialize the `.bss` section with this small snippet:

```
    mov r0, #0
    ldr r1, =_bss_start
    ldr r2, =_bss_end

bss_loop:
    cmp r1, r2
    strlt r0, [r1], #4
    blt bss_loop
```

First we store the value `0` in `R0` and then loop over memory between the addresses `_bss_start` and `_bss_end`, writing `0` to each memory address. Note how this loop is simpler than the one for `.data` - there is no ROM address stored in any registers. This is because there's no need to copy anything from ROM for `.bss`, it's all going to be zeroes anyway. Indeed, the zeroes aren't even stored in the binary because they would just take up space.

## Handing over to C

To summarize, to hand control over to C code, we need to make sure the stack is initialized, and that the memory for modifiable variables is initialized as needed. Now that our startup code handles that, there is no additional magic in how C code is started. It's just a matter of calling the `main` function in C. So we just do that in assembly and that's it:

```
    bl main
```

By using the branch-with-link instruction `bl`, we can continue running in case the `main` function returns. However, we don't actually want it to return, as it wouldn't make any sense. We want to continue running our application as long as the hardware is powered on (or QEMU is running), so a bare-metal application will have an infinite loop of some sorts. In case `main` returns, we'll just indicate an error, same as with internal CPU exceptions.

```
    b Abort_Exception

Abort_Exception:
    swi 0xFF
```

And the above branch should never execute.

# Into the C

We're finally ready to leave the complexities of linker scripts and assembly, and write some code in good old C. Create a new file, such as `cstart.c` with the following code:

```
#include <stdint.h>

volatile uint8_t* uart0 = (uint8_t*)0x10009000;

void write(const char* str)
{
	while (*str) {
		*uart0 = *str++;
	}
}

int main() {
	const char* s = "Hello world from bare-metal!\n";
	write(s);
	*uart0 = 'A';
	*uart0 = 'B';
	*uart0 = 'C';
	*uart0 = '\n';
	while (*s != '\0') {
		*uart0 = *s;
		s++;
	}
	while (1) {};

    return 0;
}
```

The code is simple and just outputs some text through the device's *Universal Asynchronous Receiver-Transmitter* (UART). The next part will discuss writing a UART driver in more detail, so let's not worry about any UART specifics for now, just note that the hardware's UART0 (there are several UARTs) control register is located at `0x10009000`, and that a single character can be printed by writing to that address.

It's pretty clear that the expected output is:

```
Hello world from bare-metal!
ABC
Hello world from bare-metal!
```

with the first line coming from the call to `write` and the other two being printed from `main`.

The more interesting thing about this code is that it tests the stack, the read-only data section and the regular data section. Let's consider how the different sections would be used when linking the above code.

```
volatile uint8_t* uart0 = (uint8_t*)0x10009000;
```

The `uart0` variable is global, not constant, and is initialized with a non-zero value. It will therefore be stored in the `.data` section, which means it will be copied to RAM by our startup code.

```
const char* s = "Hello world from bare-metal!\n";
```

Here we have a string, which will be stored in `.rodata` because that's how GCC handles most strings.

And the lines printing individual letters, like `*uart0 = 'A';` shouldn't cause anything to be stored in any of the data sections, they will just be compiled to instructions storing the corresponding character code in a register directly. The line printing `A` will do `mov r2, #0x41` when compiled, with `0x41` or `65` in decimal being the ASCII code for `A`.

Finally, the C code also uses the stack because of the `write` function. If the stack has not been set up correctly, `write` will fail to receive any arguments when called like `write(s)`, so the first line of the expected output wouldn't appear. The stack is also used to allocate the `s` pointer itself, meaning the third line wouldn't appear either.

# Building and running

There are a few changes to how we need to build the application. Assembling the startup code in `startup.s` is not going to change:

```
arm-none-eabi-as -o startup.o startup.s
```

The new C source file needs to be compiled, and a few special link options need to be passed to GCC:

```
arm-none-eabi-gcc -nostdlib -nostartfiles -lgcc -o cstart.o cstart.c
```

With `-nostdlib` we indicate that we're not using the standard C library, or any other standard libraries that GCC would like to link against. The C standard library provides the very useful standard functions like `printf`, but it also assumes that the system implements certain requirements for those functions. We have nothing of the sort, so we don't link with the C library at all. However, since `-nostdlib` disables all default libraries, we explicitly re-add `libgcc` with the `-lgcc` flag. `libgcc` doesn't provide standard C functions, but instead provides code to deal with CPU or architecture-specific issues. One such issue on ARM is that there is no ARM instruction for division, so the compiler normally has to provide a division routine, which is something GCC does in `libgcc`. We don't really need it now but include it anyway, which is good practice when compiling bare-metal ARM software with GCC.

The `-nostartfiles` option tells GCC to omit standard startup code, since we are providing our own in `startup.s`.

Compilation of `cstart.c` will produce a warning about a missing `_start` symbol. Normally, the startup code (which we skip because of `-nostartfiles`) defines a function called `_start`, but we have the reset handler in `startup.s` and it's the entry point of our program, so everything is fine - and the warning will disappear later when defining proper build targets.

Linking everything to obtain an ELF file has not undergone any changes, except for the addition of `cstart.o`:

```
arm-none-eabi-ld -T linkscript.ld -o cenv.elf startup.o cstart.o
```

Even though previously we booted a hang example through U-Boot by using an ELF file directly, now we'll need to go back to using a plain binary, which means calling `objcopy` to convert the ELF file into a binary. Why is this necessary? There's an educational aspect and the practical aspect. From the educational side, having a raw binary is much closer to the situation we would have on real hardware, where the on-board flash memory would be written with the raw binary contents. The practical aspect is that U-Boot's support of ELF files is limited. It can load an ELF into memory and boot it, but U-Boot doesn't handle ELF sections correctly, as it doesn't perform any relocation. When loading an ELF file, U-Boot just copies it to RAM and ignores how the sections should be placed. This creates problems starting with the `.text` section, which will not be in the expected memory location because U-Boot retains the ELF file's header and any padding that might exist between it and `.text`. Workarounds for these problems are possible, but using a binary file is simpler and much more reasonable.

We convert `cenv.elf` into a raw binary as follows:

```
arm-none-eabi-objcopy -O binary cenv.elf cenv.bin
```

Finally, when invoking `mkimage` to create the U-Boot image, we specify the binary as the input. After that we can create the SD card image using the same `create-sd.sh` script we made in the previous part.

```
mkimage -A arm -C none -T kernel -a 0x60000000 -e 0x60000000 -d cenv.bin bare-arm.uimg
./create-sd.sh sdcard.img bare-arm.uimg
```

That's it for building the application! All that remains is to run QEMU (remember to specify the right path to the U-Boot binary). One change in the QEMU command-line is that we will now use `-m 512M` to provide the machine with 512 megabytes of RAM. Since we're using RAM to also emulate ROM, we need the memory at `0x60000000` to be accessible, but also the memory at `0x70000000`. With those addresses being 256 megabytes apart, we need to tell QEMU to emulate at least that much memory.

```
qemu-system-arm -M vexpress-a9 -m 512M -no-reboot -nographic -monitor telnet:127.0.0.1:1234,server,nowait -kernel ../common_uboot/u-boot -sd sdcard.img -serial stdio
```

The `-serial stdio` option tells QEMU to emulate the hardware's serial output, which is the UART0 we write to, using the PC's standard I/O. That allows us to see the UART output in the terminal.

Run QEMU as above, and you should see the three lines written to UART by our C code. There's now a real program running on our ARM Versatile Express!
