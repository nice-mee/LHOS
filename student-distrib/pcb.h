#ifndef _PCB_H
#define _PCB_H
#include "types.h"
#include "filesys.h"

#define NUM_FILES 8

typedef struct {
    file_descriptor_t fd_array[NUM_FILES];
} pcb_t;

#endif /* _PCB_H */
