#define ASM     1

#define GENERATE_EXC_ASM_WRAPPER(name) ;\
.GLOBL name ;\
name: ;\
    pushl $__##name ;\
    jmp common_exception_handler

.text

ret_from_expection_handler:
ret_from_interrupt_handler:
    addl $4, %esp
    popal
    iret

/* Syscall Handling. */
syscall:
    pushl $0
    pushal
    jmp ret_from_interrupt_handler



/* Interrupt Handling. */

common_interrupt_handler:
    pushal
    cld

    jmp ret_from_interrupt_handler

intr_RTC_handler:
    pushl $0
    jmp common_interrupt_handler

intr_keyboard_handler:
    pushl $0
    jmp common_interrupt_handler

/* Exception Handling. */

common_exception_handler:
    pushal
    cld
    movl 32(%esp), %edi
    call *%edi
    jmp ret_from_expection_handler

GENERATE_EXC_ASM_WRAPPER(exc_divide_error)
GENERATE_EXC_ASM_WRAPPER(exc_debug)
GENERATE_EXC_ASM_WRAPPER(exc_nmi)
GENERATE_EXC_ASM_WRAPPER(exc_breakpoint)
GENERATE_EXC_ASM_WRAPPER(exc_overflow)
GENERATE_EXC_ASM_WRAPPER(exc_bounds)
GENERATE_EXC_ASM_WRAPPER(exc_invalid_op)
GENERATE_EXC_ASM_WRAPPER(exc_device_not_available)
GENERATE_EXC_ASM_WRAPPER(exc_double_fault)
GENERATE_EXC_ASM_WRAPPER(exc_coprocessor_segment_overrun)
GENERATE_EXC_ASM_WRAPPER(exc_invalid_TSS)
GENERATE_EXC_ASM_WRAPPER(exc_segment_not_present)
GENERATE_EXC_ASM_WRAPPER(exc_stack_fault)
GENERATE_EXC_ASM_WRAPPER(exc_general_protection)
GENERATE_EXC_ASM_WRAPPER(exc_page_fault)
GENERATE_EXC_ASM_WRAPPER(exc_FPU_error)
GENERATE_EXC_ASM_WRAPPER(exc_alignment_check)
GENERATE_EXC_ASM_WRAPPER(exc_machine_check)
GENERATE_EXC_ASM_WRAPPER(exc_SIMD_error)
