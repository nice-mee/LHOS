#include "scheduler.h"

/* set_user_PDE - Set User-Level Page Directory Entry
 *
 * Updates the memory paging structure for the new process
 *
 * Inputs: pid: The pid for which the PDE is being set.
 * Outputs: None
 * Side Effects:
 *   - Updates the page directory entry for the user process.
 *   - Flushing the TLB.
 */

static void set_user_PDE(uint32_t pid)
{
    int32_t PDE_index = _128_MB >> 22;
    page_directory[PDE_index].P    = 1;
    page_directory[PDE_index].PS   = 1;
    page_directory[PDE_index].US   = 1;
    page_directory[PDE_index].ADDR = (EIGHT_MB + pid * FOUR_MB) >> 12;

    // flushing TLB by reloading CR3 register
    asm volatile (
        "movl %%cr3, %%eax;"  // Move the value of CR3 into EBX
        "movl %%eax, %%cr3;"  // Move the value from EBX back to CR3
        : : : "eax", "memory"
    );
}

/* scheduler - Context Switching Scheduler
 *
 * Switches execution from the current terminal and process to the next in a round-robin fashion.
 *
 * Inputs: None
 * Outputs: None (performs a context switch)
 * Side Effects: 
 *   - Changes the active terminal
 *   - Switches the context to another process
 *   - Updates the memory paging structure for the new process
 *   - Modifies the TSS to point to the new process's kernel stack
 */
void scheduler() {
    
    /* save ESP and EBP */
    uint32_t cur_esp, cur_ebp;
    asm volatile("movl %%esp, %0":"=r" (cur_esp));
    asm volatile("movl %%ebp, %0":"=r" (cur_ebp));

    /*Switch to another terminal and corresponding process*/
    int next_pid = vt_set_active_term(cur_esp, cur_ebp);

    /* Remap the user program */
    set_user_PDE(next_pid);

    /* Set tss */
    tss.ss0 = KERNEL_DS;
    tss.esp0 = EIGHT_MB - next_pid * EIGHT_KB;

    uint32_t esp, ebp;
    vt_get_ebp_esp(&esp, &ebp);
    asm volatile("movl %0, %%esp;"
                 "movl %1, %%ebp;"
                 // Same technique used in halt
                 "leave;"
                 "ret;"
                :: "r"(esp),
                   "r"(ebp)
                : "esp", "ebp"
    );
}
