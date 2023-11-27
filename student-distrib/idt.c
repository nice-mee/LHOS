/* idt.c - helper functions for initializing the IDT
 * vim:ts=4 noexpandtab
 */

#include "idt.h"
#include "lib.h"

void temp_syscall_handler() {
    printf("Syscall invoked\n");
}

void inline exception_init() {
    int i;
    for (i = 0; i < NUM_EXC; i++){
		idt[i].seg_selector = KERNEL_CS;
		idt[i].reserved4 = 0;
		idt[i].size = 1;
		idt[i].reserved3 = 1;
		idt[i].reserved2 = 1;
		idt[i].reserved1 = 1;
		idt[i].reserved0 = 0;
		idt[i].dpl = 0;
		idt[i].present = 1;
	}
    SET_IDT_ENTRY(idt[0], exc_divide_error);
    SET_IDT_ENTRY(idt[1], exc_debug);
    SET_IDT_ENTRY(idt[2], exc_nmi);
    SET_IDT_ENTRY(idt[3], exc_breakpoint);
    SET_IDT_ENTRY(idt[4], exc_overflow);
    SET_IDT_ENTRY(idt[5], exc_bounds);
    SET_IDT_ENTRY(idt[6], exc_invalid_op);
    SET_IDT_ENTRY(idt[7], exc_device_not_available);
    SET_IDT_ENTRY(idt[8], exc_double_fault);
    SET_IDT_ENTRY(idt[9], exc_coprocessor_segment_overrun);
    SET_IDT_ENTRY(idt[10], exc_invalid_TSS);
    SET_IDT_ENTRY(idt[11], exc_segment_not_present);
    SET_IDT_ENTRY(idt[12], exc_stack_fault);
    SET_IDT_ENTRY(idt[13], exc_general_protection);
    SET_IDT_ENTRY(idt[14], exc_page_fault);
    // No interrupt on 15
    SET_IDT_ENTRY(idt[16], exc_FPU_error);
    SET_IDT_ENTRY(idt[17], exc_alignment_check);
    SET_IDT_ENTRY(idt[18], exc_machine_check);
    SET_IDT_ENTRY(idt[19], exc_SIMD_error);
}

void inline interrupt_init() {
    int i;
    for (i = NUM_EXC; i < NUM_VEC; i++){
		idt[i].seg_selector = KERNEL_CS;
		idt[i].reserved4 = 0;
		idt[i].size = 1;
		idt[i].reserved3 = 0;
		idt[i].reserved2 = 1;
		idt[i].reserved1 = 1;
		idt[i].reserved0 = 0;
		idt[i].dpl = 0;
		idt[i].present = 1;
	}
    SET_IDT_ENTRY(idt[PIT_VEC], intr_PIT_handler);
    SET_IDT_ENTRY(idt[KEYBOARD_VEC], intr_keyboard_handler);
    SET_IDT_ENTRY(idt[RTC_VEC], intr_RTC_handler);
}

void inline syscall_init() {
    idt[SYSCALL_VEC].seg_selector = KERNEL_CS;
    idt[SYSCALL_VEC].reserved4 = 0;
    idt[SYSCALL_VEC].size = 1;
    idt[SYSCALL_VEC].reserved3 = 1;
    idt[SYSCALL_VEC].reserved2 = 1;
    idt[SYSCALL_VEC].reserved1 = 1;
    idt[SYSCALL_VEC].reserved0 = 0;
    idt[SYSCALL_VEC].dpl = 3;
    idt[SYSCALL_VEC].present = 1;
    SET_IDT_ENTRY(idt[SYSCALL_VEC], syscall_handler);
}

void idt_init() {
    exception_init();
    interrupt_init();
    syscall_init();
    lidt(idt_desc_ptr);
}

