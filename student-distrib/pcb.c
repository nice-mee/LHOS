#include "pcb.h"

#define EIGHT_MB 0x800000
#define EIGHT_KB 0x2000
#define EIGHT_KB_MASK 0xFFFFE000

/* get_pcb_by_pid - get the pcb by pid
 * Inputs: pid - the given pid
 * Outputs: the pcb with the given pid
 * Side Effects: None
 */
pcb_t* get_pcb_by_pid(uint32_t pid)
{
    return EIGHT_MB - (pid + 1) * EIGHT_KB;
}

/* get_current_pcb - get the current pcb
 * Inputs: None
 * Outputs: the current pcb
 * Side Effects: None
 */
pcb_t* get_current_pcb()
{
    uint32_t esp;
    asm volatile("movl %%esp, %0" : "=r"(esp));
    return (pcb_t*)(esp & EIGHT_KB_MASK);
}

/* get_current_pid - get the current pid
 * Inputs: None
 * Outputs: the current pid
 * Side Effects: None
 */
int32_t get_current_pid()
{
    uint32_t esp;
    asm volatile("movl %%esp, %0" : "=r"(esp));
    return (EIGHT_MB - esp) / EIGHT_KB;
}
