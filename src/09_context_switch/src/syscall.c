#include <stdint.h>

#define LR_REG_OFFSET (14)

enum syscalls {
    SYSCALL_ENDTASK = 0
};

void syscall_handler(uint32_t syscall, uint32_t *regs) {
    switch (syscall) {
    
    case SYSCALL_ENDTASK:
        asm ("mov r3, 60 \n\t"
        "add sp, r3 \n\t"
        "ldr r3, [r1, #56] \n\t" //LR is regs[14]
        "mov pc, r3");
        break;
    }
}
