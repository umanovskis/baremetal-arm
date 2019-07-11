#include "systime.h"
#include "uart_pl011.h"

static volatile systime_t systime;

void systime_tick(void) {
    systime++;
}

systime_t systime_get(void) {
    return systime;
}
