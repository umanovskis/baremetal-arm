#include <stdint.h>

typedef uint32_t systime_t;
typedef int(*systime_callback)(void*);

typedef enum {
    SYSTIME_CALLBACK_OK = 0,
    SYSTIME_NO_CALLBACK_SLOTS
} systime_callback_error;

void systime_tick(void);
systime_t systime_get(void);
systime_callback_error systime_schedule_event(systime_t timestamp, systime_t period, systime_callback callback, void* arg);
