#include "systime.h"

typedef void (*task_entry_ptr)(void);

typedef struct {
    task_entry_ptr entry;
    systime_t period;
    systime_t last_run;
} task_desc;

typedef enum {
    SCHED_OK = 0,
    SCHED_TOO_MANY_TASKS
} sched_error;

#define MAX_NUM_TASKS (10u)

sched_error sched_add_task(task_entry_ptr entry, systime_t period);
void sched_run(void);
