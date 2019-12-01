#include <stddef.h>
#include "sched_preemptive.h"

static task_desc task_table[MAX_NUM_TASKS] = {0};
static uint8_t table_idx = 0;

static task_context csa[MAX_NUM_TASKS] = {0};
//static process_context* current_context;
static task_desc* current_task;

sched_error sched_add_task(task_entry_ptr entry, systime_t period) {
    if (table_idx >= MAX_NUM_TASKS) {
        return SCHED_TOO_MANY_TASKS;
    }

    task_desc task = {
        .id = table_idx,
        .entry = entry,
        .period = period,
        .last_run = 0,
        .context = &csa[table_idx]
    };
    task_table[table_idx++] = task;

    return SCHED_OK;
}

static void save_context(task_context* ctx) {
    asm volatile(
    ".arm \t\n"
    "stmia r0!, {r0-r14}^ \t\n"
    );
}

static int task_switch_callback(void* arg) {
    uint8_t new_task_id = *((uint8_t*)arg);
    if (current_task && (new_task_id == current_task->id)) {
        return 0;
    }
    uart_write("Switching context! Time ");
    uart_write_uint(systime_get());
    uart_write("; ");
    if (current_task) {
        uart_write_uint(current_task->id);
    } else {
        uart_write("(idle)");
    }
    uart_write(" --> ");
    uart_write_uint(new_task_id);
    uart_write("\n");
    if (current_task) {
        save_context(&csa[current_task->id]);
    }
    current_task = &(task_table[new_task_id]);

    return 0;
}

static void activate_task(task_entry_ptr fn) {
    asm("mov r4, 0x1F");
    asm("msr cpsr_c, r4");
    fn();
    asm("mov r4, 0x13");
    asm("msr cpsr_c, r4");
}

void sched_run(void) {
    for (uint8_t i = 0; i < MAX_NUM_TASKS; i++) {
       task_desc* task = &task_table[i];
       if (task->entry == NULL) {
            continue;
       }
       systime_t next_run = task->last_run + task->period;
       systime_schedule_event(next_run, task->period, task_switch_callback, &task->id);
    }
    current_task = &task_table[0]; // Simplification: always start the first task added
    while(1) {
        if (current_task) {
            activate_task(current_task->entry);
            current_task = NULL;
        }
    }
}
