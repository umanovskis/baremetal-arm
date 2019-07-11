#include "tasks.h"
#include "uart_pl011.h"
#include "systime.h"
#include <stdio.h>

void task1(void) {
    systime_t start = systime_get();
    uart_write("Entering task 1... systime ");
    uart_write_uint(start);
    uart_write("\n");
    while (start + 1000u > systime_get());
    uart_write("Exiting task 1...\n");
}

void task2(void) {
    systime_t start = systime_get();
    uart_write("Entering task 2... systime ");
    uart_write_uint(start);
    uart_write("\n");
    while (start + 1000u > systime_get());
    uart_write("Exiting task 2...\n");
}

void task3(void) {
    systime_t start = systime_get();
    uart_write("Entering task 3... systime ");
    uart_write_uint(start);
    uart_write("\n");
    while(1);
}
