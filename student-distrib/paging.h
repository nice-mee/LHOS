/* paging.h - Defines for various paging constants, functions, and structs
 * controller
 * vim:ts=4 noexpandtab
 */

#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"

#define PAGE_SIZE 4096
#define DIR_TBL_SIZE 1024
#define PAGE_TBL_SIZE 1024
#define VID_MEM_ADDR 0xB8000
#define KERNEL_ADDR 0x400000


/*
 * The following structs define the page directory and page table entries.
 * The way of defining with reference to:
 * https://wiki.osdev.org/Paging#Page_Directory
 */
typedef struct page_directory_entry {
    uint32_t P   : 1; // Present
    uint32_t RW  : 1; // Read/Write
    uint32_t US  : 1; // User/Supervisor
    uint32_t PWT : 1; // Write-Through
    uint32_t PCD : 1; // Cache Disable
    uint32_t A   : 1; // Accessed
    uint32_t D   : 1; // Dirty
    uint32_t PS  : 1; // Page Size
    uint32_t G   : 1; // Global
    uint32_t AVL : 3; // Available
    uint32_t ADDR: 20;// Address
} PDE_t;

typedef struct page_table_entry {
    uint32_t P   : 1; // Present
    uint32_t RW  : 1; // Read/Write
    uint32_t US  : 1; // User/Supervisor
    uint32_t PWT : 1; // Write-Through
    uint32_t PCD : 1; // Cache Disable
    uint32_t A   : 1; // Accessed
    uint32_t D   : 1; // Dirty
    uint32_t PAT : 1; // Page Attribute Table
    uint32_t G   : 1; // Global
    uint32_t AVL : 3; // Available
    uint32_t ADDR: 20;// Address
} PTE_t;

// Define the page directory and page table.
PDE_t page_directory[DIR_TBL_SIZE] __attribute__((aligned(PAGE_SIZE)));
PTE_t page_table[DIR_TBL_SIZE] __attribute__((aligned(PAGE_SIZE)));

void paging_init();

#endif /* _PAGING_H */
