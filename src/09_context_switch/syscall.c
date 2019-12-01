#include <stdint.h>

enum syscalls {
    SYSCALL_ZERO = 0
};

void syscall_handler(uint32_t syscall, uint32_t *regs) {
}
