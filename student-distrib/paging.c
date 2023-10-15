/* paging.c - Functions to help set up paging
 * vim:ts=4 noexpandtab
 */

#include "paging.h"
#include "lib.h"

void paging_init(){
    
    memset(page_table, 0, sizeof(PTE_t) * DIR_TBL_SIZE);
    memset(page_directory, 0, sizeof(PDE_t) * DIR_TBL_SIZE);

    // Initialize the page table.
    int pos = (VID_MEM_ADDR >> 12);
    page_table[pos].P    = 1;
    page_table[pos].RW   = 1;
    page_table[pos].US   = 0;
    page_table[pos].PWT  = 0;
    page_table[pos].PCD  = 0;
    page_table[pos].A    = 0;
    page_table[pos].D    = 0;
    page_table[pos].PAT  = 0;
    page_table[pos].G    = 1; // ???
    page_table[pos].AVL  = 0;
    page_table[pos].ADDR = pos;

    // Initialize the page directory for kernel.
    page_directory[1].P    = 1;
    page_directory[1].RW   = 1;
    page_directory[1].US   = 0;
    page_directory[1].PWT  = 0;
    page_directory[1].PCD  = 1; // ???
    page_directory[1].A    = 0;
    page_directory[1].D    = 0;
    page_directory[1].PS   = 1;
    page_directory[1].G    = 1;
    page_directory[1].AVL  = 0;
    page_directory[1].ADDR = KERNEL_ADDR >> 12;

    // Initialize the page directory for page table.
    page_directory[0].P    = 1;
    page_directory[0].RW   = 1;
    page_directory[0].US   = 0;
    page_directory[0].PWT  = 0; // ???
    page_directory[0].PCD  = 0;
    page_directory[0].A    = 0;
    page_directory[0].D    = 0;
    page_directory[0].PS   = 0;
    page_directory[0].G    = 0;
    page_directory[0].AVL  = 0;
    page_directory[0].ADDR = (uint32_t)page_table >> 12;

    asm volatile(
        "movl %0, %%eax;"
        "movl %%eax, %%cr3;"
        "movl %%cr4, %%eax;"
        "orl $0x00000010, %%eax;"
        "movl %%eax, %%cr4;"
        "movl %%cr0, %%eax;"
        "orl $0x80000000, %%eax;"
        "movl %%eax, %%cr0;"
        :
        : "r"(page_directory)
        : "eax"
    );
}
