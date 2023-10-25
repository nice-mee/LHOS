#include "pcb.h"

#define EIGHT_MB 0x800000
#define EIGHT_KB 0x2000

pcb_t* get_pcb_by_pid(uint32_t pid)
{
    return EIGHT_MB - (pid + 1) * EIGHT_KB;
}

pcb_t* get_current_pcb()
{
    int32_t current_pid = get_current_pid();
    return get_pcb_by_pid(current_pid);
}

int32_t* get_current_pid()
{
    
}
