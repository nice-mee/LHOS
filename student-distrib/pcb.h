#ifndef _PCB_H
#define _PCB_H
#include "types.h"
#include "filesys.h"

#define NUM_FILES 8
#define MAX_PID_NUM 2
#define FOUR_MB 0x400000
#define EIGHT_MB 0x800000
#define EIGHT_KB 0x2000
#define EIGHT_KB_MASK 0xFFFFE000
#define _128_MB 0x8000000
#define USER_STACK_START (_128_MB + FOUR_MB)
#define EXECUTABLE_START 0x08048000


typedef struct pcb_s pcb_t;
struct pcb_s {
    uint32_t pid;
    file_descriptor_t fd_array[NUM_FILES];
    pcb_t* parent_pcb;
    uint32_t esp;
    uint32_t ebp;
};

extern pcb_t* get_pcb_by_pid(uint32_t pid);
extern pcb_t* get_current_pcb();

extern int32_t get_current_pid();
extern int32_t get_available_pid();
extern int32_t free_pid(int32_t pid);

#endif /* _PCB_H */
