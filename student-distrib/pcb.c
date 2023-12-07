#include "pcb.h"

static int32_t pid_occupied[MAX_PID_NUM] = {0,};

/* get_pcb_by_pid - get the pcb by pid
 * Inputs: pid - the given pid
 * Outputs: the pcb with the given pid
 * Side Effects: None
 */
pcb_t* get_pcb_by_pid(uint32_t pid)
{
    return (pcb_t *)(EIGHT_MB - (pid + 1) * EIGHT_KB);
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

/* get_available_pid - get the available pid
 * Inputs: None
 * Outputs: the available pid
 * Side Effects: None
 */
int32_t get_available_pid()
{
    int32_t i;
    for (i = 0; i < MAX_PID_NUM; i++) {
        if (pid_occupied[i] == 0) {
            pid_occupied[i] = 1;
            return i;
        }
    }
    // No pid available
    return -1;
}

/* free_pid - free the pid
 * Inputs: pid - the given pid
 * Outputs: 0 if success, -1 if fail
 * Side Effects: None
 */
int32_t free_pid(int32_t pid)
{
    if (pid < 0 || pid >= MAX_PID_NUM) {
        return -1;
    }
    pid_occupied[pid] = 0;
    return 0;
}

int32_t check_pid_occupied(int32_t pid)
{
    return pid_occupied[pid];
}
