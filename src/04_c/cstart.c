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
