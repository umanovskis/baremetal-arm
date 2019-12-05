#include <stdint.h>

#define LR_REG_OFFSET (14)

enum syscalls {
    SYSCALL_ENDTASK = 0
};

void syscall_handler(uint32_t syscall, uint32_t *regs) {
    switch (syscall) {
    
    case SYSCALL_ENDTASK: ;
        uint32_t next_instr = regs[LR_REG_OFFSET];
        asm("mov pc, %0" : : "r"(next_instr));
        break;
    }
}
