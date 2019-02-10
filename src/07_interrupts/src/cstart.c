#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "uart_pl011.h"
#include "cpu.h"
#include "gic.h"

int main() {
        uart_config config = {
            .data_bits = 8,
            .stop_bits = 1,
            .parity = false,
            .baudrate = 9600
        };
        uart_configure(&config);

        uart_putchar('A');
        uart_putchar('B');
        uart_putchar('C');
        uart_putchar('\n');

        uart_write("Welcome to Chapter 7, Interrupts!\n");
	gic_init();
	gic_enable_interrupt(UART0_INTERRUPT);
	cpu_enable_interrupts();

        while (1) { }

        return 0;
}

void __attribute__((interrupt)) irq_handler(void) {
        uint16_t irq = gic_acknowledge_interrupt();
        switch (irq) {
        case UART0_INTERRUPT:
            uart_isr();
            break;
        default:
            uart_write("Unknown interrupt!\n");
            break;
        }
        gic_end_interrupt(irq);
}

