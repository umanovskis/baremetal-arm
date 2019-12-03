/* Some defines */
.equ MODE_USR, 0x10
.equ MODE_FIQ, 0x11
.equ MODE_IRQ, 0x12
.equ MODE_SVC, 0x13
.equ MODE_SYS, 0x1F

.section .vector_table, "x"
.global _Reset
.global _start
_Reset:
    b Reset_Handler
    b Abort_Exception /* 0x4  Undefined Instruction */
    b SVC_Handler_Top /* 0x8  Software Interrupt */
    b Abort_Exception  /* 0xC  Prefetch Abort */
    b Abort_Exception /* 0x10 Data Abort */
    b . /* 0x14 Reserved */
    b irq_handler /* 0x18 IRQ */
    b . /* 0x1C FIQ */

.section .text
Reset_Handler:
    /* Change the vector table base address */
    ldr r0, =0x60000000
    mcr p15, #0, r0, c12, c0, #0

    /* FIQ stack */
    msr cpsr_c, MODE_FIQ
    ldr r1, =_fiq_stack_start
    ldr sp, =_fiq_stack_end
    movw r0, #0xFEFE
    movt r0, #0xFEFE

fiq_loop:
    cmp r1, sp
    strlt r0, [r1], #4
    blt fiq_loop

    /* IRQ stack */
    msr cpsr_c, MODE_IRQ
    ldr r1, =_irq_stack_start
    ldr sp, =_irq_stack_end

irq_loop:
    cmp r1, sp
    strlt r0, [r1], #4
    blt irq_loop

    /* Supervisor mode */
    msr cpsr_c, MODE_SVC
    ldr r1, =_stack_start
    ldr sp, =_stack_end

stack_loop:
    cmp r1, sp
    strlt r0, [r1], #4
    blt stack_loop

    /* Start copying data */
    ldr r0, =_text_end
    ldr r1, =_data_start
    ldr r2, =_data_end

    /* User mode stack */
    msr cpsr_c, MODE_SYS
    ldr r1, =_usr_stack_start
    ldr sp, =_usr_stack_end

usr_loop:
    cmp r1, sp
    strlt r0, [r1], #4
    blt usr_loop
    msr cpsr_c, MODE_SVC

data_loop:
    cmp r1, r2
    ldrlt r3, [r0], #4
    strlt r3, [r1], #4
    blt data_loop

    /* Initialize .bss */
    mov r0, #0
    ldr r1, =_bss_start
    ldr r2, =_bss_end

bss_loop:
    cmp r1, r2
    strlt r0, [r1], #4
    blt bss_loop

    /* Disable supervisor mode interrupts */
    cpsid if

/* Test code starts below - see the section on
 the supervisor call handler in chapter 9 for an
 explanation. */

    msr cpsr_c, MODE_SYS
    mov r0, #0xa0
    mov r1, #0xa1
    mov r10, #0xaa
    mov r11, #0xab
test1:
    svc 0x42
after_svc:
    nop
    bl test2
    msr cpsr_c, MODE_SVC

.func test2
.thumb_func
test2:
    .thumb
    svc 0xbb
    nop
    bx lr
.endfunc
.arm

/* Test code ends here */

    bl main
    b Abort_Exception

Abort_Exception:
    swi 0xFF

.global IrqHandler
IrqHandler:
    ldr r0, =0x10009000
    ldr r1, [r0]
    b .

SVC_Handler_Top:
    push {r0-r12, lr}
    mrs r0, spsr
    push {r0} // save spsr to stack

    tst r0, #0x20 // Thumb mode?
    ldrneh r0, [lr, #-2]
    bicne r0, r0, #0xFF00
    ldreq r0, [lr, #-4]
    biceq r0, r0, #0xFF000000

    mov r1, sp
    //bl swc_handler
    pop {r0} // load spsr from stack
    msr spsr_cxsf, r0
    ldmfd sp!, {r0-r12, pc}^
