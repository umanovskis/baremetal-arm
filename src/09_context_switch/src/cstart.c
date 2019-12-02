#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "uart_pl011.h"
#include "cpu.h"
#include "gic.h"
#include "ptimer.h"
#include "tasks.h"
#include "sched.h"

int cb(void* arg) {
    uart_write("Callback!\n");
    uart_write_uint(systime_get());
    uart_write("\n");
    uart_write_uint(*(int*)arg);
    uart_write("\n");
}

int main() {
        uart_config config = {
            .data_bits = 8,
            .stop_bits = 1,
            .parity = false,
            .baudrate = 9600
        };
        uart_configure(&config);

        uart_write("Welcome to Chapter 9, Context Switch!\n");
	gic_init();
	gic_enable_interrupt(UART0_INTERRUPT);
	gic_enable_interrupt(PTIMER_INTERRUPT);
	cpu_enable_interrupts();
        //cpu_enter_user_mode();

	if (ptimer_init(1u) != PTIMER_OK) {
	    uart_write("Failed to initialize CPU timer!\n");
	}

        (void)sched_add_task(&task0, 5000u);
        (void)sched_add_task(&task1, 2000u);

        sched_run();

        return 0;
}

