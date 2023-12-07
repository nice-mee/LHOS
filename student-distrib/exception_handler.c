/* exception_handler.c - Actual exception handlers
 * vim:ts=4 noexpandtab
 */
#include "exception_handler.h"

// For debugging purpose only
// extern void __exc_divide_error()
// {
//     printf("fuckfuck\n");
//     while(1); 
// }

extern void __exc_divide_error()
{
    printf("Exception 0x%x: " "divide by zero" "\n" , 0);
    send_signal(SIGNUM_DIV_ZERO);
}

extern void __exc_page_fault()
{
    printf("Exception 0x%x: " "page fault" "\n" , 0);
    send_signal(SIGNUM_SEGFAULT);
}

GENERATE_EXCEPTION_HANDLER(1, "debug", exc_debug)
GENERATE_EXCEPTION_HANDLER(2, "non-maskable interrupt", exc_nmi)
GENERATE_EXCEPTION_HANDLER(3, "breakpoint", exc_breakpoint)
GENERATE_EXCEPTION_HANDLER(4, "overflow", exc_overflow)
GENERATE_EXCEPTION_HANDLER(5, "bound range exceeded", exc_bounds)
GENERATE_EXCEPTION_HANDLER(6, "invalid opcode", exc_invalid_op)
GENERATE_EXCEPTION_HANDLER(7, "device not available", exc_device_not_available)
GENERATE_EXCEPTION_HANDLER(8, "double fault", exc_double_fault)
GENERATE_EXCEPTION_HANDLER(9, "coprocessor segment overrun", exc_coprocessor_segment_overrun)
GENERATE_EXCEPTION_HANDLER(10, "invalid TSS", exc_invalid_TSS)
GENERATE_EXCEPTION_HANDLER(11, "segment not present", exc_segment_not_present)
GENERATE_EXCEPTION_HANDLER(12, "stack segment fault", exc_stack_fault)
GENERATE_EXCEPTION_HANDLER(13, "general protection", exc_general_protection)
GENERATE_EXCEPTION_HANDLER(16, "floating point error", exc_FPU_error)
GENERATE_EXCEPTION_HANDLER(17, "alignment check", exc_alignment_check)
GENERATE_EXCEPTION_HANDLER(18, "machine check", exc_machine_check)
GENERATE_EXCEPTION_HANDLER(19, "SIMD floating point error", exc_SIMD_error)
