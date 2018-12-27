#ifndef CPU_A9_H
#define CPU_A9_H

#include <stdint.h>

inline uint32_t cpu_get_periphbase() {
    uint32_t result;
    asm ("mrc p15, #4, %0, c15, c0, #0" : "=r" (result));
    return result;
}

#endif
