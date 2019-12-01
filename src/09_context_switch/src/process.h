typedef uint32_t* stack_pointer;
typedef uint32_t cpu_register;

typedef struct {
    stack_pointer stack_top;
    cpu_register registers[16]; /* R0 to R15 */
} process_context;
