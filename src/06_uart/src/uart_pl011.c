#include <stdbool.h>
#include <math.h>
#include "uart_pl011.h"

static uart_registers* uart0 = (uart_registers*)0x10009000u;
const uint32_t refclock = 24000000u; /* 24 MHz */

uart_error uart_init(void) {
    while (uart0->FR & FR_BUSY);
    uart0->LCRH &= ~LCRH_FEN;

    return UART_OK;
}

uart_error uart_configure(uart_config* config) {
    /* Validate config */
    if (config->data_bits < 5u || config->data_bits > 8u) {
        return UART_INVALID_ARGUMENT_WORDSIZE;
    }
    if (config->stop_bits == 0u || config->stop_bits > 2u) {
        return UART_INVALID_ARGUMENT_STOP_BITS;
    }
    if (config->baudrate < 110u || config->baudrate > 460800u) {
        return UART_INVALID_ARGUMENT_BAUDRATE;
    }

    /* Set baudrate */
    double intpart, fractpart;
    double baudrate_divisor = refclock / (16u * config->baudrate);
    fractpart = modf(baudrate_divisor, &intpart);

    uart0->IBRD = (uint16_t)intpart;
    uart0->FBRD = (fractpart * 64u) + 0.5;

    /* Set data word size */
    switch (config->data_bits)
    {
    case 5:
        uart0->LCRH |= LCRH_WLEN_5BITS;
        break;
    case 6:
        uart0->LCRH |= LCRH_WLEN_6BITS;
        break;
    case 7:
        uart0->LCRH |= LCRH_WLEN_7BITS;
        break;
    case 8:
        uart0->LCRH |= LCRH_WLEN_8BITS;
        break;
    }

    /* Set parity. If enabled, use even parity */
    if (config->parity) {
        uart0->LCRH |= LCRH_PEN;
        uart0->LCRH |= LCRH_EPS;
        uart0->LCRH |= LCRH_SPS;
    } else {
        uart0->LCRH &= ~LCRH_PEN;
        uart0->LCRH &= ~LCRH_EPS;
        uart0->LCRH &= ~LCRH_SPS;
    }

    /* Set stop bits */
    if (config->stop_bits == 1u) {
        uart0->LCRH &= ~LCRH_STP2;
    } else if (config->stop_bits == 2u) {
        uart0->LCRH |= LCRH_STP2;
    }

    /* Enable the UART */
    uart0->CR |= CR_UARTEN;

    return UART_OK;
}

void uart_putchar(char c) {
    uart0->DR = c;
}

void uart_write(const char* data) {
    while (*data) {
        uart0->DR = *data++;
    }
}
