#include <stdint.h>
#include <stdbool.h>
#include "uart_pl011.h"

int main() {
	uart_init();
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
	uart_write("I love drivers!\n");
	while (1) {};

	return 0;
}
