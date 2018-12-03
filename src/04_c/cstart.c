#include <stdint.h>

void write(const char* str)
{
	volatile uint8_t* uart0 = (uint8_t*)0x10009000;
	char *s = str;
	while (*s) {
		*uart0 = *s++;
	}
}

int main(int argc, char** argv) {
	volatile uint8_t* uart2 = (uint8_t*)0x10009000;
	const char* s = "Hello world more more text so more!\n";
	write(s);
	*uart2 = 'A';
	*uart2 = 'B';
	*uart2 = 'C';
	*uart2 = '\n';
	while (*s != '\0') {
		*uart2 = *s;
		s++;
	}
	while (1) {};
}
