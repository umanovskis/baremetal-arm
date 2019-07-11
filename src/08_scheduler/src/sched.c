#include <stddef.h>
#include <stdint.h>
#include "sched.h"

static task_desc task_table[MAX_NUM_TASKS] = {0};
static int table_idx = 0;

sched_error sched_add_task(task_entry_ptr entry, systime_t period) {
    if (table_idx >= MAX_NUM_TASKS) {
        return SCHED_TOO_MANY_TASKS;
    }

    task_desc task = {
        .entry = entry,
        .period = period,
        .last_run = 0
    };
    task_table[table_idx++] = task;

    return SCHED_OK;
}

void sched_run(void) {
    while (1) {
        for (uint8_t i = 0; i < MAX_NUM_TASKS; i++) {
            task_desc* task = &task_table[i];
            if (task->entry == NULL) {
                continue;
            }

            if (task->last_run + task->period <= systime_get()) { /* Overflow bug! */
                task->last_run = systime_get();
                task->entry();
            }
        }
    }
}
