#ifndef _PCB_H
#define _PCB_H
#include "types.h"
#include "filesys.h"

#define NUM_FILES 8

typedef struct {
    file_descriptor_t fd_array[NUM_FILES];
} pcb_t;

extern pcb_t* get_pcb_by_pid(uint32_t pid);
extern pcb_t* get_current_pcb();

extern int32_t get_current_pid();

#endif /* _PCB_H */
