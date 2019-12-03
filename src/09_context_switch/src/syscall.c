#include <stdint.h>

enum syscalls {
    SYSCALL_ENDTASK = 0
};

void syscall_handler(uint32_t syscall, uint32_t *regs) {
    switch (syscall) {
    
    case SYSCALL_ENDTASK:
        break;
    }
}
