/* filesys.h - Defines the read-only file system
 * vim:ts=4 noexpandtab
 */

#ifndef _DYNAMIC_ALLOC_H
#define _DYNAMIC_ALLOC_H

#include "types.h"

/* define basic constant for the dynamic allocation system */
#define DYNAMIC_MEMORY_START (_128_MB + FOUR_MB * 3)    // start at 140 MB
#define DYNAMIC_MEMORY_BLOCK_SIZE 4096                  // 4kB per dynamic block
#define DYNAMIC_MEMORY_SIZE FOUR_MB                     // has total 4 MB size

/* define the dynamic memory node structure */
typedef struct dynamic_allocation_node {
    struct dynamic_allocation_node* prev;
    struct dynamic_allocation_node* next;
    int32_t start_index;                // mark the corresponding start index of the PTE index
    int32_t end_index;                  // mark the corresponding end index of the PTE index
    uint32_t used_bytes_num;
    uint32_t available_bytes_num;
    uint8_t* ptr;
} dynamic_allocation_node_t;

/* functoins used by dynamic allocation system */
void dynamic_allocation_init(void);
void* malloc(int32_t size);
int32_t free(void* ptr);

#endif /* _DYNAMIC_ALLOC_H */
