#define ASM     1

#define GENERATE_EXC_ASM_WRAPPER(name) ;\
.globl name ;\
name: ;\
    pushl $__##name ;\
    jmp common_exception_handler

.text

ret_from_expection_handler:
ret_from_interrupt_handler:
    popal
    addl $4, %esp
    iret

/* Syscall Handling. */
.globl syscall_handler
syscall_handler:
    pushl %esp
    pushl %ebp
    pushl %edi
    pushl %esi
    pushfl
    pushl %edx
    pushl %ecx
    pushl %ebx

    cmpl $0, %eax
    jle arg_error
    cmpl $15, %eax
    jg arg_error
    call *syscall_table(,%eax,4)
    jmp ret_from_syscall_handler
arg_error:
    movl $-1, %eax
ret_from_syscall_handler:
    popl %edx
    popl %ecx
    popl %ebx
    popfl
    popl %esi
    popl %edi
    popl %ebp
    popl %esp
    iret

syscall_table:
    .long 0x0
    .long __syscall_halt
    .long __syscall_execute
    .long __syscall_read
    .long __syscall_write
    .long __syscall_open
    .long __syscall_close
    .long __syscall_getargs
    .long __syscall_vidmap
    .long __syscall_set_handler
    .long __syscall_sigreturn
    .long __syscall_malloc
    .long __syscall_free
    .long __syscall_ioctl
    .long __syscall_ps
    .long __syscall_date


/* Interrupt Handling. */

common_interrupt_handler:
    pushal
    cld
    movl 32(%esp), %edi
    cmpl $0, %edi
    jl handle_interrupt
    call temp_syscall_handler
    jmp ret_from_interrupt_handler
handle_interrupt:
    notl %edi
    call *interrupts(,%edi,4)
    jmp ret_from_interrupt_handler

.globl intr_RTC_handler
intr_RTC_handler:
    pushl $~0x08
    jmp common_interrupt_handler

.globl intr_keyboard_handler
intr_keyboard_handler:
    pushl $~0x01
    jmp common_interrupt_handler

.globl intr_PIT_handler
intr_PIT_handler:
    pushl $~0x00
    jmp common_interrupt_handler

interrupts:
    .long __intr_PIT_handler
    .long __intr_keyboard_handler
    .long 0 # Padding
    .long 0 # Padding
    .long 0 # Padding
    .long 0 # Padding
    .long 0 # Padding
    .long 0 # Padding
    .long __intr_RTC_handler

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
