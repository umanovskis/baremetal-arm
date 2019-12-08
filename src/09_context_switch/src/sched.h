#include <stdint.h>
#include "systime.h"

typedef uint32_t* stack_pointer;
typedef uint32_t cpu_register;

/*typedef struct {
    stack_pointer stack_top;
    cpu_register registers[16]; /* R0 to R15 */
/* } task_context;*/

typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t sp;
    uint32_t lr;
    uint32_t pc;
    uint32_t spsr;
} task_context;

typedef void (*task_entry_ptr)(void);

typedef struct {
    uint8_t id;
    task_entry_ptr entry;
    systime_t period;
    systime_t last_run;
    task_context* context;
} task_desc;

typedef enum {
    SCHED_OK = 0,
    SCHED_TOO_MANY_TASKS
} sched_error;

#define MAX_NUM_TASKS (10u)

sched_error sched_add_task(task_entry_ptr entry, systime_t period);
void sched_run(void);

void sched_end_task(uint32_t);
