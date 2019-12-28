#include <stddef.h>
#include "sched.h"

static task_desc task_table[MAX_NUM_TASKS] = {0};
static uint8_t table_idx = 0;

static task_context csa[MAX_NUM_TASKS] = {0};
//static process_context* current_context;
static task_desc* current_task;
static uint8_t current_task_id = MAX_NUM_TASKS;

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
//    asm volatile(
//    ".arm \t\n"
//    "stmia r0!, {r0-r14}^ \t\n"
//    );
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
    current_task_id = new_task_id;

    return 0;
}

static uint32_t saved_regs[12];
static uint32_t saved_sp;

static void run_task(task_entry_ptr entry) {
    uint32_t mode;
    uint32_t* regs = saved_regs;
    uint32_t cpsr;

    asm("mrs r1, cpsr \n\t"
        "push {r1, lr} \n\t"
        "bic r1, r1, 0x3 \n\t"
        "msr cpsr, r1"
        );

    entry();
    asm("svc 0 \n\t"
    "pop {r0, lr} \n\t"
    "msr cpsr, r0");
}

void sched_end_task(uint32_t next) {
    uint32_t sp_local = saved_sp;
    asm("mov sp, %0" : : "r"(sp_local));
    asm("mov pc, r0");
    return;
    asm("pop {r1-r7, lr}");
    asm("mov pc, r0");
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
    current_task_id = 0;
    while(1) {
        if (current_task) {
            run_task(current_task->entry);
            current_task = NULL;
            current_task_id = MAX_NUM_TASKS;
        }
    }
}

void context_switch(uint8_t new_id) {
    if (current_task_id != MAX_NUM_TASKS) {
        uint32_t irq_sp;
        uint32_t* csa = csa[current_task_id];
        asm("mov r1, 0x14 \n\t"
            "msr cpsr, r1 \n\t"
            "pop {r1 - r6, r12, lr} \n\t"
            "mov r2, 0x13 \n\t"
            "msr cpsr, r2 \n\t");
    }
}

void sched_choose(void) {
    uint8_t candidate = MAX_NUM_TASKS;
    for (uint8_t i = 0; i < MAX_NUM_TASKS; i++) {
       task_desc* task = &task_table[i];
       if (task->entry == NULL) {
            continue;
       }
       systime_t next_run = task->last_run + task->period;
       if (next_run <= systime_get() && current_task != &task_table[i]) {
            candidate = i;
       }
    }
    if (candidate != MAX_NUM_TASKS) {
        context_switch(candidate);
    }
}
