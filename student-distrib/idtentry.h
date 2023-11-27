/* idtentry.h - 
 * vim:ts=4 noexpandtab
 */


extern void exc_divide_error();
extern void exc_debug();
extern void exc_nmi();
extern void exc_breakpoint();
extern void exc_overflow();
extern void exc_bounds();
extern void exc_invalid_op();
extern void exc_device_not_available();
extern void exc_double_fault();
extern void exc_coprocessor_segment_overrun();
extern void exc_invalid_TSS();
extern void exc_segment_not_present();
extern void exc_stack_fault();
extern void exc_general_protection();
extern void exc_page_fault();
extern void exc_FPU_error();
extern void exc_alignment_check();
extern void exc_machine_check();
extern void exc_SIMD_error();

extern void syscall_handler();

extern void intr_RTC_handler();
extern void intr_keyboard_handler();
extern void intr_PIT_handler();
