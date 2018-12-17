# Driver development for UART

This chapter will concern driver development, a crucial part of bare-metal programming. We will walk through writing a UART driver for the Versatile Express series, but the ambition here is not so much to cover that particular UART in detail as it is to show the general approach and patterns when writing a similar driver. As always with programming, there is a lot that can be debated, and there are parts that can be done differently. Starting with a UART driver specifically has its advantages. UARTs are very common peripherals, they're much simpler than other serial buses such as SPI or I2C, and the UART pretty much corresponds to standard input/output when run in QEMU.

# Doing the homework

Writing a peripheral driver is not something you should just jump into. You need to understand the device itself, and how it integrates with the rest of the hardware. If you start coding off the hip, you're likely to end up with major design issues, or just a driver that mysteriously fails to work because you missed a small but crucial detail. Thinking before doing should apply to most programming, but driver programming is particularly unforgiving if you fail to follow that rule.

Before writing a peripheral device driver, we need to understand, in broad strokes, the following about the device:

* How it performs its function(s). Whether it's a communication device, a signal converter, or anything else, there are going to be many details of how the device operates. In the case of a UART device, some of the things that fall here are, what baud rates does it support? Are there input and output buffers? When does it sample incoming data?

* How it is controlled. Most of the time, the peripheral device will have several registers, writing and reading them is what controls the device. You need to know what the registers do.

* How it integrates with the hardware. When the device is part of a larger system, which could be a system-on-a-chip or a motherboard-based design, it somehow connects to the rest of the system. Does the device take an external input clock and, if so, where from? Does enabling the device require some other system conditions to be met? The registers for controlling the device are somehow accessible from the CPU, typically by being mapped to a particular memory address. From a CPU perspective, registers that control peripherals are *Special Function Registers* (SFR), though not all SFRs correspond to peripherals.

Let's then look at the UART of the Versatile Express and learn enough about it to be ready to create the driver.

## Basic UART operation

UART is a fairly simple communications bus. Data is sent out on one wire, and received on another. Two UARTs can thus communicate directly, and there is no clock signal or synchronization of any kind, it's instead expected that both UARTs are configured to use the same baud rate. UART data is transmitted in packets, which always begin with a start bit, followed by 5 to 9 data bits, then optionally a parity bit, then 1 or 2 stop bits. A packet could look like this:

```
+-------+---+---+---+---+---+---+---+---+---+---+
| Start | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | P | S |
| bit   |   |   |   |   |   |   |   |   |   |   |
|       |   |   |   |   |   |   |   |   |   |   |
+-------+---+---+---+---+---+---+---+---+---+---+
```

A pair of UARTs needs to use the same frame formatting for successful communication. In practice, the most common format is 8 bits of data, no parity bit, and 1 stop bit. This is sometimes written as `8-N-1` in shorthand, and can be written together with the baud rate like `115200/8-N-1` to describe a UART's configuration.

On to the specific UART device that we have. The Versatile Express hardware series comes with the PrimeCell UART PL011, the [reference manual](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0183g/index.html) is also available from the ARM website. Reading through the manual, we can see that the PL011 is a typical UART with programmable baud rate and packet format settings, that it supports infrared transmission, and FIFO buffers both for transmission and reception. Additionally there's Direct Memory Access (DMA) support, and support for hardware flow control. In the context of UART, hardware flow control means making use of two additional physical signals so that one UART can inform another when it's ready to send, or ready to receive, data.

## Key PL011 registers

The PL011 UART manual also describes the registers that control the peripheral, and we can identify the most important registers that our driver will need to access in order to make the UART work. We will need to work with:

* Data register (DR). The data received, or to be transmitted, will be accessed through DR.

* Receive status / error clear register (RSRECR). Errors are indicated here, and the error flag can also be cleared from here.

* Flag register (FR). Various flags indicating the UART's status are collected in FR.

* Integer baud rate register (IBRD) and Fractional baud rate register (FBRD). Used together to set the baud rate.

* Line control register (LCR_H). Used primarily to program the frame format.

* Control register (CR). Global control of the peripheral, including turning it on and off.

In addition, there are registers related to interrupts, but we will begin by using polling, which is inefficient but simpler. We will also not care about the DMA feature.

As is often the case, reading register descriptions in the manual also reveals some special considerations that apply to the particular hardware. For example, it turns out that writing the IBRD or FBRD registers will not actually have any effect until writing the LCR_H - so even if you only want to update IBRD, you need to perform a sequence of two writes, one to IBRD and another to LCR_H. It is very common in embedded programming to run into such special rules for reading or writing registers, which is one of the reasons reading the manual for the device you're about to program is so important.

## PL011 - Versatile Express integration

Now that we are somewhat familiar with the PL011 UART peripheral itself, it's time to look at how integrates with the Versatile Express hardware. The VE hardware itself consists of a motherboard and daughter board, and the UARTs are on the motherboard, which is called the Motherboard Express µATX and of course has [its own reference manual](http://infocenter.arm.com/help/topic/com.arm.doc.dui0447j/DUI0447J_v2m_p1_trm.pdf).

One important thing from the PL011 manual is the reference clock, UARTCLK. Some peripherals have their own independent clock, but most of them, especially simpler peripherals, use an external reference clock that is then often divided further as needed. For external clocks, the peripheral's manual cannot provide specifics, so the information on what the clock is has to be found elsewhere. In our case, the motherboard's documentation has a separate section on clocks (*2.3 Clock architecture* in the linked PDF), where we can see that UARTs are clocked by the OSC2 clock from the motherboard, which has a frequency of 24 MHz. This is very convenient, we will not need to worry about the reference clock possibly having different values, we can just say it's 24 MHz.

Next we need to find where the UART SFRs are located from the CPU's perspective. The motherboard's manual has memory maps, which differ depending on the daughter board. We're using the CoreTile Express A9x4, so it has what the manual calls the *ARM Legacy memory map* in section 4.2. It says that the address for UART0 is `0x9000`, using SMB (System Memory Bus) chip select CS7, with the chip select introducing an additional offset that the daughter board defines. Then it's on to the [CoreTile Express A9x4 manual](http://infocenter.arm.com/help/topic/com.arm.doc.dui0448i/DUI0448I_v2p_ca9_trm.pdf), which explains that the board's memory controller places most motherboard peripherals under CS7, and in *3.2 Daughterboard memory map* we see that CS7 memory mappings for accessing the motherboard's peripherals start at `0x10000000`. Thus the address of UART0 from the perspective of the CPU running our code is CS7 base `0x10000000` plus an offset of `0x9000`, so `0x10009000` is ultimately the address we need.

Yes, this means that we have to check two different manuals just to find the peripheral's address. This is, once again, nothing unusual in an embedded context.

# Writing the driver

## What's in the box?

In higher-level programming, you can usually treat drivers as a black box, if you even give them any consideration. They're there, they do things with hardware, and they only have a few functions you're exposed to. Now that we're writing a driver, we have to consider what it consists of, the things we need to implement. Broadly, we can say that a driver has:

* An initialization function. It starts the device, performing whatever steps are needed. This is usually relatively simple.

* Configuration functions. Most devices can be configured to perform their functions differently. For a UART, programming the baud rate and frame format would fall here. Configuration can be simple or very complex.

* Runtime functions. These are the reason for having the driver in the first place, the interesting stuff happens here. In the case of UART, this means functions to transmit and read data.

* A deinitialization function. It turns the device off, and is quite often omitted.

* Interrupt handlers. Most peripherals have some interrupts, which need to be handled in special functions called interrupt handlers, or interrupt service routines. We won't be covering that for now.

Now we have a rough outline of what we need to implement. We will need code to start and configure the UART, and to send and receive data. Let's get on with the implementation.

## Exposing the SFRs

We know by now that programming the UART will be done by accessing the SFRs. It is possible, of course, to access the memory locations directly, but a better way is to define a C struct that reflects what the SFRs look like. We again refer to the PL011 manual for the register summary. It begins like this:

![PL011 register summary](images/06_regsummary.png)

Looking at the table, we can define it in code as follows:

```
typedef volatile struct __attribute__((packed)) {
    uint32_t DR;                /* 0x0 Data Register */
    uint32_t RSRECR;            /* 0x4 Receive status / error clear register */
    uint32_t _reserved0[4];     /* 0x8 - 0x14 reserved */
    const uint32_t FR;          /* 0x18 Flag register */
    uint32_t _reserved1;        /* 0x1C reserved */
    uint32_t ILPR;              /* 0x20 Low-power counter register */
    uint32_t IBRD;              /* 0x24 Integer baudrate register */
    uint32_t FBRD;              /* 0x28 Fractional baudrate register */
    uint32_t LCRH;              /* 0x2C Line control register */
    uint32_t CR;                /* 0x30 Control register */
} uart_registers;
```

There are several things to note about the code. One is that it uses fixed-size types like `uint32_t`. Since the C99 standard was adopted, C has included the `stdint.h` header that defines exact-width integer types. So `uint32_t` is guaranteed to be a 32-bit type, as opposed to `unsigned int`, for which there is no guaranteed fixed size. The layout and size of the SFRs is fixed, as described in the manual, so the struct has to match in terms of field sizes.

For the same reason, `__attribute__((packed))` is provided. Normally, the compiler is allowed to insert padding between struct fields in order to align the whole struct to some size suited for the architecture. Consider the following example:

```
typedef struct {
    char a; /* 1 byte */
    int b;  /* 4 bytes */
    char c; /* 1 byte */
} example;
```

If you compile that struct for a typical x86 system where `int` is 4 bytes, the compiler will probably try to align the struct to a 4-byte boundary, and align the individual members to such a boundary as well, inserting 3 bytes after `a` and another 3 bytes after `c`, giving the struct a total size of 12 bytes.

When working with SFRs, we definitely don't want the compiler to insert any padding bytes or take any other liberties with the code. `__attribute__((packed))` is a GCC attribute (also recognized by some other compilers like clang) that tells the compiler to use the struct as it is written, using the least amount of memory possible to represent it. Forcing structs to be packed is generally not a great idea when working with "normal" data, but it's very good practice for structs that represent SFRs.

Sometimes there might be reserved memory locations between various registers. In our case of the PL011 UART, there are reserved bytes between the `RSRECR` and `FR` registers, and four more after `FR`. There's no general way in C to mark such struct fields as unusable, so giving them names like `_reserved0` indicates the purpose. In our struct definition, we define `uint32_t _reserved0[4];` to skip 16 bytes, and `uint32_t _reserved1;` to skip another 4 bytes later.

Some SFRs are read-only, like the `FR`, in which case it's helpful to declare the corresponding field as `const`. Attempts to write a read-only register would fail anyway (the register would remain unchanged), but marking it as `const` lets the compiler check for attempts to write the register.
