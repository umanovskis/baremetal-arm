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

This is the loop that actually fills the stack with `0xFEFEFEFE`. First it compares the value in R1 to the value in SP. If R1 is less than SP, the value in R0 will be written to the address stored in R1, and R1 gets increased by 4. Then the loop continues as long as R1 is less than SP. 

Once the loop is over and the FIQ stack is ready, we repeat the process with the IRQ stack, and finally the supervisor mode stack (see the code in the full listing of `startup.s` at the end).
