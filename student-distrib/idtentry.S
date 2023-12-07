#define ASM     1

#define GENERATE_EXC_ASM_WRAPPER(name) ;\
.globl name ;\
name: ;\
    subl $4, %esp ;\
    pushl %fs ;\
    pushl %es ;\
    pushl %ds ;\
    pushl %eax ;\
    pushl %ebp ;\
    pushl %edi ;\
    pushl %esi ;\
    pushl %edx ;\
    pushl %ecx ;\
    pushl %ebx ;\
    call __##name ;\
    call handle_signal ;\
    popl %ebx ;\
    popl %ecx ;\
    popl %edx ;\
    popl %esi ;\
    popl %edi ;\
    popl %ebp ;\
    popl %eax ;\
    popl %ds ;\
    popl %es ;\
    popl %fs ;\
    addl $4, %esp ;\
    iret

#define GENERATE_EXC_ASM_WRAPPER_ERRNO(name) ;\
.globl name ;\
name: ;\
    pushl %fs ;\
    pushl %es ;\
    pushl %ds ;\
    pushl %eax ;\
    pushl %ebp ;\
    pushl %edi ;\
    pushl %esi ;\
    pushl %edx ;\
    pushl %ecx ;\
    pushl %ebx ;\
    call __##name ;\
    call handle_signal ;\
    popl %ebx ;\
    popl %ecx ;\
    popl %edx ;\
    popl %esi ;\
    popl %edi ;\
    popl %ebp ;\
    popl %eax ;\
    popl %ds ;\
    popl %es ;\
    popl %fs ;\
    addl $4, %esp ;\
    iret

#define GENERATE_INTR_ASM_WRAPPER(name) ;\
.globl name ;\
name: ;\
    subl $4, %esp ;\
    pushl %fs ;\
    pushl %es ;\
    pushl %ds ;\
    pushl %eax ;\
    pushl %ebp ;\
    pushl %edi ;\
    pushl %esi ;\
    pushl %edx ;\
    pushl %ecx ;\
    pushl %ebx ;\
    call __##name ;\
    call handle_signal ;\
    popl %ebx ;\
    popl %ecx ;\
    popl %edx ;\
    popl %esi ;\
    popl %edi ;\
    popl %ebp ;\
    popl %eax ;\
    popl %ds ;\
    popl %es ;\
    popl %fs ;\
    addl $4, %esp ;\
    iret


.text

/* Syscall Handling. */
.globl syscall_handler
syscall_handler:
    subl $4, %esp
    pushl %fs
    pushl %es
    pushl %ds
    pushl %eax
    pushl %ebp
    pushl %edi
    pushl %esi
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
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp
    addl $4, %esp
    popl %ds
    popl %es
    popl %fs
    addl $4, %esp
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

GENERATE_EXC_ASM_WRAPPER(exc_divide_error)
GENERATE_EXC_ASM_WRAPPER(exc_debug)
GENERATE_EXC_ASM_WRAPPER(exc_nmi)
GENERATE_EXC_ASM_WRAPPER(exc_breakpoint)
GENERATE_EXC_ASM_WRAPPER(exc_overflow)
GENERATE_EXC_ASM_WRAPPER(exc_bounds)
GENERATE_EXC_ASM_WRAPPER(exc_invalid_op)
GENERATE_EXC_ASM_WRAPPER(exc_device_not_available)
GENERATE_EXC_ASM_WRAPPER_ERRNO(exc_double_fault)
GENERATE_EXC_ASM_WRAPPER(exc_coprocessor_segment_overrun)
GENERATE_EXC_ASM_WRAPPER_ERRNO(exc_invalid_TSS)
GENERATE_EXC_ASM_WRAPPER_ERRNO(exc_segment_not_present)
GENERATE_EXC_ASM_WRAPPER_ERRNO(exc_stack_fault)
GENERATE_EXC_ASM_WRAPPER_ERRNO(exc_general_protection)
GENERATE_EXC_ASM_WRAPPER_ERRNO(exc_page_fault)
GENERATE_EXC_ASM_WRAPPER(exc_FPU_error)
GENERATE_EXC_ASM_WRAPPER_ERRNO(exc_alignment_check)
GENERATE_EXC_ASM_WRAPPER(exc_machine_check)
GENERATE_EXC_ASM_WRAPPER(exc_SIMD_error)

GENERATE_INTR_ASM_WRAPPER(intr_PIT_handler)
GENERATE_INTR_ASM_WRAPPER(intr_keyboard_handler)
GENERATE_INTR_ASM_WRAPPER(intr_RTC_handler)
