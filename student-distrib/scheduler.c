#include "scheduler.h"


void scheduler() {
    /*Switch to another terminal and corresponding process*/
    int nxt_pid = vt_set_active_term();

    /* Remap the user program */
    set_user_prog_page(next_pid);

    /* Set tss */
    tss.ss0 = KERNEL_DS;
    tss.esp0 = EIGHT_MB - nxt_pid * EIGHT_KB;

    pcb_t* next_pcb = get_pcb(next_pid);
    asm volatile("movl %%esp, %0;"
                 "movl %%ebp, %1;"
                : "=r"(parent_pcb->esp),
                  "=r"(parent_pcb->ebp)
                : : "memory");
}