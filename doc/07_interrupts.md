# Interrupts

There's no reasonable way of handling systems programming, such as embedded development or operating system development, without interrupts being a major consideration.

What's an interrupt, anyway? It's a kind of notification signal that a CPU receives as an indication that something important needs to be handled. An interrupt is often sent by another hardware device, in which case that's a *hardware interrupt*. The CPU responds to an interrupt by interrupting its current activity (hence the name), and switching to a special function called an *interrupt handler* or an *interrupt service routine* - ISR for short. After dealing with the interrupt, the CPU will resume whatever it was doing previously.

There are also *software interrupts*, which can be triggered by the CPU itself upon detecting an error, or may be possible for the programmer to trigger with code.

Interrupts are used primarily for performance reasons. A polling-based approach, where external devices are continuously asked for some kind of status, are inefficient. The UART driver we wrote in the previous chapter is a prime example of that. We use it to let the user send data to our program by typing, and typing is far slower than the frequency at which the CPU can check for new data. Interrupts solve this problem by instead letting the device notify the CPU of an event, such as the UART receiving a new data byte.

If you read the manual for the PL011 UART in the previous chapter, you probably remember seeing some registers that control interrupt settings, which we ignored at the time. So, changing the driver to work with interrupts should be just a matter of setting some registers to enable interrupts and writing an ISR to handle them, right?

No, not even close. Inerrupt handling is often quite complicated, and there's work to be done before any interrupts can be used at all, and then there are additional considerations for any ISRs. Let's get to it.

## Interrupt handling in ARMv7-A

Interrupt handling is very hardware-dependent. We need to look into the general interrupt handling procedure of the particular architecture, and then into specifics of a particular implementation like a specific CPU. The ARMv7-A manual provides quite a lot of useful information about interrupt handling on that architecture.

ARMv7-A uses the generic term *exception* to refer, in general terms, to interrupts and some other exception types like CPU errors. An interrupt is called an *IRQ exception* in ARMv7-A, so that's the term the manual names a lot.When an ARMv7-A CPU takes an exception, it transfers control to an instruction located at the appropriate location in the vector table, depending on the exception type. The very first code we wrote for startup began with the vector table.

As a reminder:

```
_Reset:
    b Reset_Handler
    b . /* 0x4  Undefined Instruction */
    b . /* 0x8  Software Interrupt */
    b . /* 0xC  Prefetch Abort */
    b . /* 0x10 Data Abort */
    b . /* 0x14 Reserved */
    b . /* 0x18 IRQ */
```

In the code above, we had an instruction at offset `0x0`, for the reset exception, and dead loops for the other exception types, including IRQs at `0x18`. So normally, an ARMv7-A CPU will execute the instruction at `0x18` starting from the vector table's beginning when it takes an IRQ exception.

There's more that happens, too. When an IRQ exception is taken, the CPU will switch its mode to the IRQ mode, affecting how some registers are seen by the CPU. When we were initially preparing the stack for the C environment, we set several stacks up for different CPU modes, IRQ mode being one of them.

Ignoring some advanced ARMv7 features like monitor and hypervisor modes, the sequence upon taking an IRQ exception is the following:

1. Figure out the address of the next instruction to be executed after handling the interrupt, and write it into the `LR` register.

2. Save the `CPSR` register, which contains the current processor status, into the `SPSR` register.

3. Switch to IRQ mode, by changing the mode bits in `CPSR` to `0x12`.

4. Make some additional changes in `CPSR`, such as clearing conditional execution flags.

5. Check the `VE` (Interrupt Vectors Enabled) bit in the `SCTLR` (system control register). If `VE` is `0`, go to the start of the vector table plus `0x18`. If `VE` is `1`, go to the appropriate implementation-defined location for the interrupt vector.

That last part sounds confusing. What's with that implementation-defined location?

Remember that ARMv7-A is not a CPU. It's a CPU architecture. In this architecture, interrupts are supported as just discussed, and there's always the possibility to use an interrupt handler at `0x18` bytes into the vector table. That is, however, not always convenient. Consider that there can be many different interrupt sources, while the vector table can only contain one branch instruction at `0x18`. This means that the the function taking care of interrupts would first have to figure out which interrupt was triggered, and then act appropriately. Such an approach puts extra burden on the CPU as it has to check all possible interrupt sources.

The solution to that is known as vectored interrupts. In a vectored interrupt system, each interrupt has its own vector (a unique ID). Then some kind of interrupt controller is in place that knows which ISR to route each interrupt vector to.

The ARMv7-A architecture has numerous implementations, as in specific CPUs. The architecture description says that vectored interrupts may be supported, but the details are left up to the implementation. The choice of which interrupt system to use, though, is controlled by the architecture-defined `SCTLR` register.

We want to use vectored interrupts. Mentally noting the fact that `SCTRL` needs to have the `VE` bit set to `1`, it's time to turn to the manual for the specific implementation we're programming for.

## Generic Interrupt Controller of the Cortex-A9

We're programming for a CoreTile Express A9x4 daughterboard, which contains the Cortex-A9 MPCore CPU. The MPCore means it's a CPU that can consist of one to four individual Cortex-A9 cores. So it's the [Cortex-A9 MPCore manual](https://static.docs.arm.com/ddi0407/h/DDI0407H_cortex_a9_mpcore_r4p0_trm.pdf) that becomes our next stop. There's a chapter in the manual for the interrupt controller - so far so good - but it immediately refers to another manual. Turns out that the Cortex-A9 has an interrupt controller of the *ARM Generic Interrupt Controller* type, for which there's a separate manual (note that GIC version 4.0 makes a lot of references to the ARMv8 architecture). The Cortex-A9 manual refers to version 1.0 of the GIC specification, but reading version 2.0 is also fine, there aren't too many differences and none in the basic features.

The GIC has its own set of SFRs that control its operation, and the GIC as a whole is responsible for forwarding interrupt requests to the correct A9 core in the A9-MPCore. There are two main components in the GIC - the Distributor and the CPU interfaces. The Distributor receives interrupt requests, prioritizes them, and forwards them to the CPU interfaces, each of which corresponds to an A9 core.

Let's clarify with a schematic drawing. The Distributor and the CPU interfaces are all part of the GIC, with each CPU then using its own assigned CPU interface to communicate with the GIC. The communication is two-way because CPUs need to not only receive interrupts but also, at least, to inform the GIC when interrupt handling completes.

```
                     ARM GIC
  IRQ source  +------------------------+
+-------------> +----------+           |
              | |          | +-------+ |   +-----------+
  IRQ source  | | Distrib- | | CPU   +-----> Cortex A-9|
+-------------> | utor     | | I-face| |   |           |
              | |          | | 0     | |   | CPU 0     |
  IRQ source  | |          | |       <-----+           |
+-------------> |          | +-------+ |   +-----------+
              | |          |           |
  IRQ source  | |          | +-------+ |
+-------------> |          | | CPU   | |   +-----------+
              | +----------+ | I-face+-----> Cortex A-9|
  IRQ source  |              | 1     | |   |           |
+------------->              |       <-----+ CPU 1     |
              |              +-------+ |   |           |
              +------------------------+   +-----------+
```

To enable interrupts, we'll need to program the GIC Distributor, telling it to enable certain interrupts, and forward them to our CPU. Once we have some form of working interrupt handling, we'll need to tell our program to report back to the GIC, using the CPU Interface part, when the handling of an interrupt has been finished.

The general sequence for an interrupt is as follows:

1. The GIC receives an interrupt request. That particular interrupt is now considered *pending*.

2. If the specific interrupt is enabled in the GIC, the Distributor determines the core or cores to forward it to.

3. Among all pending interrupts, the Distributor chooses the one with the highest priority for each CPU interface.

4. The GIC's CPU interface forwards the interrupt to the processor, if priority rules tell it to do so.

5. The processor acknowledges the interrupt, informing the GIC. The interrupt is now *active* or, possibly, *active and pending* if the interrupt has been requested again.

6. The software running on the processor handles the interrupt and then informs the GIC that the handling is complete. The interrupt is now *inactive*.

Note that interrupts can also be preempted, that is, a higher-priority interrupt can be forwarded to a CPU while it's already processing an active lower-priority interrupt.

Just as with the UART driver previously, it's wise to identify some key registers of the GIC that we will need to program to process interrupts. I'll once again omit the `GIC` prefix in register names for brevity. Registers whose names start with `D` (or `GICD` in full) belong to the Distributor system, those with `C` names belong to the CPU interface system.

For the Distributor, key registers include:

* DCTLR - the global Distributor Control Register, containing the enable bit - no interrupts will be forwarded to CPUs without turning that bit on.

* DISENABLERn - interrupt set-enable registers. There are multiple such registers, hence the n at the end. Writing to these registers enables specific interrupts.

* DICENABLERn - interrupt clear-enable registers. Like the above, but writing to these registers disables interrupts.

* DIPRIORITYRn - interrupt priorty registers. Lets each interrupt have a different priority level, with these priorities determining which interrupt actually gets forwarded to a CPU when there are multiple pending interrupts.

* DITARGETSRn - interrupt processor target registers. These determine which CPU will get notified for each interrupt.

* DICFGRn - interrupt configuration registers. They identify whether each interrupt is edge-triggered or level-sensitive. Edge-triggered interrupts can be deasserted (marked as no longer pending) by the peripheral that triggered them in the first place, level-sensitive interrupts can only be cleared by the CPU.

There are more Distributor registers but the ones above would let us get some interrupt handling in place. That's just the Distributor part of the GIC though, there's also the CPU interface part, with key registers including:

* CCTLR - CPU interface control register, enabling or disabling interrupt forwarding to the particular CPU connected to that interface.

* CCPMR - interrupt priority mask register. Acts as a filter of sorts between the Distributor and the CPUs - this register defines the minimum priority level for an intrrupt to be forwarded to the CPU.

* CIAR - interrupt acknowledge register. The CPU receiving the interrupt is expected to read from this register in order to obtain the interrupt ID, and thereby acknowledge the interrupt.

* CEOIR - end of interrupt register. The CPU is expected to write to this register after completing the handling of an interrupt.
