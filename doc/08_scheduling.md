# Scheduling

Very few embedded applications can be useful without some kind of time-keeping and scheduling. Being able to program things like "do this every X seconds" or "measure the time between events A and B" is a key aspect of nearly all practically useful applications.

In this chapter, we'll look at two related concepts. *Timers*, which are a hardware feature that allows software to keep track of time, and *scheduling*, which is how you program a system to run some code, or a *task*, on some kind of time-based schedule - hence the name. Scheduling in particular is a complex subject, some discussion of which will follow later, but first we'll need to set up some kind of time measurement system.

In order to keep track of time in our system, we're going to use two tiers of "ticks". First, closer to the hardware, we'll have a timer driver for a hardware timer of the Cortex-A9 CPU. This driver will generate interrupts at regular intervals. We will use those interrupts to keep track of *system time*, a separate counter that we'll use in the rest of the system as "the time".

Such a split is not necessary in a simple tutorial system, but is good practice due to the system time thus not being directly connected to a particular hardware clock or driver implementation. This allows better portability as it becomes possible to switch the underlying timer driver without affecting uses of system time.

The first task then is to create a timer driver. Since its purpose will be to generate regular interrupts, note that this work builds directly on the previous chapter, where interrupt handling capability was added.

## Private Timer Driver

A Cortex-A9 MPCore CPU provides a global timer and private timers. There's one private timer per core. The global timer is constantly counting up, even with the CPU paused in debug mode. The per-core private timers count down from some starting value to zero, sending an interrupt when zero is reached. It's possible to use either timer for scheduling, but the typical solution is to use the private timer. It's somewhat easier to handle due to being 32 bits wide (the global timer is 64 bits) and due to stopping when the CPU is stopped.

The Cortex-A9 MPCore manual explains the private timer in Chapter 4.1. The timer's operation is quite simple. A certain starting *load value* is loaded into the load register `LR`. When the timer is started, it keeps counting down from that load value, and generates an interrupt when the counter reaches `0`. Then, if the auto-reload function is enabled, the counter automatically restarts from the load value. As is common with other devices, the private timer has its own control register `CTRL`, which controls whether auto-reload is enabled, whether interrupt generation is enabled, and whether the whole timer is enabled.

From the same manual, we can see that the private timer's registers are at offset `0x600` from `PERIPHBASE`, and we already used `PERIPHBASE` in the previous chapter for GIC registers. Finally, the manual gives the formula to calculate the timer's period.

![Private timer period formula](images/08_timerperiod.png)

A prescaler can essentially reduce the incoming clock frequency, but using that is optional. If we simplify with the assumption that prescaler is `0`, we can get `Load value = (period * PERIPHCLK) - 1`. The peripheral clock, `PERIPHCLK`, is the same 24 MHz clock from the motherboard that clocks the UART.

I will not go through the timer driver in every detail here, as it just applies concepts from the previous two chapters. As always, you can examine the full source in this chapter's corresponding source code folder. We call the driver `ptimer`, implement it in `ptimer.h` and `ptimer.c`, and the initialization function is as follows:

```
ptimer_error ptimer_init(uint16_t millisecs) {
    regs = (private_timer_registers*)PTIMER_BASE;
    if (!validate_config(millisecs)) {
        return PTIMER_INVALID_PERIOD;
    }
    uint32_t load_val = millisecs_to_timer_value(millisecs);
    WRITE32(regs->LR, load_val); /* Load the initial timer value */

    /* Register the interrupt */
    (void)irq_register_isr(PTIMER_INTERRUPT, ptimer_isr);

    uint32_t ctrl = CTRL_AUTORELOAD | CTRL_IRQ_ENABLE | CTRL_ENABLE;
    WRITE32(regs->CTRL, ctrl); /* Configure and start the timer */

    return PTIMER_OK;
}
```

The function accepts the desired timer tick period, in milliseconds, calculates the corresponding load value for `LR` and then enables interrupt generation, auto-reload and starts the timer.

One bit of interest here is the ISR registration, for which the interrupt number is defined as `#define PTIMER_INTERRUPT    (29u)`. The CPU manual says that the private timer generates interrupt `29` when the counter reaches zero. And in the code we're actually using `29`, unlike with the UART driver, where we mapped `UART0INTR`, number `5`, to `37` in the code. But this interrupt remapping only applies to certain interrupts that are generated on the motherboard. The private timer is part of the A9 CPU itself, so no remapping is needed.

The `millisecs_to_timer_value` function calculates the value to be written into `LR` from the desired timer frequency in milliseconds. Normally it should look like this:

```
static uint32_t millisecs_to_timer_value(uint16_t millisecs) {
    double period = millisecs * 0.001;
    return (period * refclock) - 1;
}
```

However, things are quite different for us due to using QEMU. It doesn't emulate the 24 MHz peripheral clock, and QEMU in general does not attempt to provide timings that are similar to the hardware being emulated. For our UART driver, this means that the baud rate settings don't have any real effect, but that wasn't a problem. For the timer though, it means that the period won't be the same as on real hardware, so the actual implementation used for this tutorial is:

```
static uint32_t millisecs_to_timer_value(uint16_t millisecs) {
    double period = millisecs * 0.001;
    uint32_t value = (period * refclock) - 1;
    value *= 3; /* Additional QEMU slowdown factor */

    return value;
}
```

With the timer driver in place, the simplest way to test is to make the timer ISR print something out, and initialize the timer with a one-second period. Here's the straightforward ISR:

```
void ptimer_isr(void) {
    uart_write("Ptimer!\n");
    WRITE32(regs->ISR, ISR_CLEAR); /* Clear the interrupt */
}
```

And somewhere in our `main` function:

```
gic_enable_interrupt(PTIMER_INTERRUPT);

if (ptimer_init(1000u) != PTIMER_OK) {
    uart_write("Failed to initialize CPU timer!\n");
}
```

Note the call to `gic_enable_interrupt`, and recall that each interrupt needs to be enabled in the GIC, it's not enough to just register a handler with `irq_register_isr`. This code should result in the private timer printing out a message every second or, due to the very approximate calculation in the emulated version, approximately every second.

Build everything and run (if you're implementing yourself as you read, remember to add the new C file to `CMakeLists.txt`), and you should see regular outputs from the timer.

---

**HINT**

You can use `gawk`, the GNU version of `awk` to print timestamps to the terminal. Instead of just `make run`, type `make run | gawk '{ print strftime("[%H:%M:%S]"), $0 }'` and you'll see the local time before every line of output. This is useful to ascertain that the private timer, when set to `1000` milliseconds, procudes output roughly every second.

---

## System Time

As discussed previously, we want to use some kind of *system time* system-wide. This is going to have a very straightforward implementation. The private timer will tick every millisecond, and its ISR will increment the system time. So system time itself will also be measured in milliseconds. Then `systime.c` is exceedingly simple:

```
#include "systime.h"

static volatile systime_t systime;

void systime_tick(void) {
    systime++;
}

systime_t systime_get(void) {
    return systime;
}
```

The `systime_t` type is defined in the corresponding header file, as `typedef uint32_t systime_t`.

To make use of this, the private timer's ISR is modified so it simply calls `systime_tick` after clearing the hardware interrupt flag.

```
void ptimer_isr(void) {
    WRITE32(regs->ISR, ISR_CLEAR); /* Clear the interrupt */
    systime_tick();
}
```

That's it, just change the `ptimer_init` call in `main` to use a 1-millisecond period, and you have a system-wide time that can be accessed whenever needed.

## Overflows and spaceships

A discussion of timers is an opportune time to not only make a bad pun but also to mention overflows. By no means limited to embedded programming, overflows are nonetheless more prominent in low-level systems programming. As a refresher, an overflow occurs when the result of a calculation exceeds the maximum range of its datatype. A `uint8_t` has the maximum value `255` and so `255 + 1` would cause an overflow.

Timers in particular tend to overflow. For example, our use of `uint32_t` for system time means that the maximum system timer value is `0xFF FF FF FF`, or just shy of 4.3 billion in decimal. A timer that ticks every millisecond will reach that number after 49 days. So code that assumes a timer will always keep inceasing can break in mysterious ways after 49 days. This kind of bug is notoriously difficult to track to down.

One solution is of course to use a bigger data type. Using a 64-bit integer to represent a millisecond timer would be sufficient for 292 billion years. This does little to address problems in older systems, however. Many UNIX-based systems begin counting time from the 1st of January, 1970, and use a 32-bit integer, giving rise to what's known as the Year 2038 problem, as such systems cannot represent any time after January 19, 2038.

When overflows are possible, code should account for them. Sometimes overflows can be disregarded, but saying that something "cannot happen" is dangerous. It's reasonable to assume, for example, that a microwave oven won't be running for 49 days in a row, but in some circumstances such assumptions should not be made.

One example of an expensive, irrecoverable overflow bug is the NASA spacecraft *Deep Impact*. After more than eight years in space, and multiple significant accomplishments including excavating a comet, *Deep Impact* suddenly lost contact with Earth. That was due to a 32-bit timer overflowing and causing the onboard computers to continuously reboot.

Overflow bugs can go unnoticed for many years. The binary search algorithm, which is very widely used, is often implemented incorrectly due to an overflow bug, and that bug was not noticed for two decades, in which it even made its way into the Java language's standard library.

## Scheduler types

A scheduler is responsible for allocating necessary resources to do some work. To make various bits of code run on a schedule, CPU time is the resource to be allocated, and the various tasks comprise work. Different types of schedulers and different scheduling algorithms exist, with a specific choice depending on the use case and the system's constraints.

One useful concept to understand is that of *real-time systems*. Such a system has constraints, or *deadlines*, on some timings, and these deadlines are expressed in specific time measurements. A "fast response" isn't specific, "response within 2 milliseconds" is. Further, a real-time system is said to be *hard real-time* if it cannot be allowed to miss any deadlines at all, that is, a single missed deadline constitutes a system failure. Real-time systems are commonly found in embedded systems controlling aircraft, cars, industrial or medical equipment, and so on.By contrast, most consumer-oriented software isn't real-time.

A real-time system requires a scheduler that can guarantee adherence to the required deadlines. Such systems will typically run a real-time operating system (RTOS) which provides the necessary scheduling support. We're not dealing with real-time constraints, and we're not writing an operating system, so putting RTOS aside, there are two classes of schedulers to consider.

*Cooperative schedulers* provide *cooperative (or non-preemptive) multitasking*. In this case, every task is allowed to run until it returns control to the scheduler, at which point the scheduler can start another task. Cooperative scedulers are easy to implement, but their major downside is relying on each task to make reasonable use of CPU resources. A poorly-written task can cause the entire system to slow down or even hang entirely. Implementing individual tasks is also simpler in the cooperative case - the task can assume that it will not be interrupted, and will instead run from start to finish.

Cooperative scheduling is fairly common in low-resource embedded systems, and the implementation only requires that some kind of system-wide time exists.

*Preemptive schedulers* use interrupts to *preempt*, or suspend, a task and hand control over to another task. This ensures that one task is not able to hang the entire system, or just take too long before letting other tasks run. Such schedulers implement some kind of algorithm for choosing when to interrupt a task and what task to execute next, and implementing actual preemption is another challenge.

## Cooperative scheduler


