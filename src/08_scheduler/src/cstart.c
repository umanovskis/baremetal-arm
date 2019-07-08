#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "uart_pl011.h"
#include "cpu.h"
#include "gic.h"
#include "ptimer.h"

int main() {
        uart_config config = {
            .data_bits = 8,
            .stop_bits = 1,
            .parity = false,
            .baudrate = 9600
        };
        uart_configure(&config);

        uart_write("Welcome to Chapter 8, Scheduling!\n");
	gic_init();
	gic_enable_interrupt(UART0_INTERRUPT);
	gic_enable_interrupt(PTIMER_INTERRUPT);
	cpu_enable_interrupts();

	if (ptimer_init(1000u) != PTIMER_OK) {
	    uart_write("Failed to initialize CPU timer!\n");
	}

        while (1) { }

        return 0;
}

