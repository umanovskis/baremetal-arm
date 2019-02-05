#ifndef CPU_H
#define CPU_H

#ifdef CPU_A9
#include "cpu_a9.h"
#else
#error No CPU defined in build parameters!
#endif

#endif
