#ifndef GIC_H
#define GIC_H

#include <stdint.h>

typedef struct {
    uint32_t DCTLR;                 /* 0x0 Distributor Control register */
    const uint32_t DTYPER;          /* 0x4 Controller type register */
    const uint32_t DIIDR;           /* 0x8 Implementer identification register */
    uint32_t _reserved0[29];        /* 0xC - 0x80; reserved and implementation-defined */
    uint32_t DIGROUPR[32];          /* 0x80 - 0xFC Interrupt group registers */
    uint32_t DISENABLER[32];        /* 0x100 - 0x17C Interrupt set-enable registers */
    uint32_t DICENABLER[32];        /* 0x180 - 0x1FC Interrupt clear-enable registers */
    uint32_t DISPENDR[32];          /* 0x200 - 0x27C Interrupt set-pending registers */
    uint32_t DICPENDR[32];          /* 0x280 - 0x2FC Interrupt clear-pending registers */
    uint32_t DICPENDR[32];          /* 0x280 - 0x2FC Interrupt clear-pending registers */
    uint32_t DICDABR[32];           /* 0x300 - 0x3FC Active Bit Registers (GIC v1) */
    uint32_t _reserved1[32];        /* 0x380 - 0x3FC reserved on GIC v1 */
    uint32_t DIPRIORITY[255];       /* 0x400 - 0x7F8 Interrupt priority registers */
    uint32_t _reserved2;            /* 0x7FC reserved */
    const uint32_t DITARGETSRO[8];  /* 0x800 - 0x81C Interrupt CPU targets, RO */
    uint32_t DITARGETSR[246];       /* 0x820 - 0xBF8 Interrupt CPU targets */
    uint32_t _reserved3;            /* 0xBFC reserved */
    uint32_t DICFGR[64];            /* 0xC00 - 0xCFC Interrupt config registers */
    /* Some PPI, SPI status registers and identification registers beyond this.
       Don't care about them */
} gic_distributor_registers;

#endif
