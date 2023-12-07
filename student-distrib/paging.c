/* paging.c - Functions to help set up paging
 * vim:ts=4 noexpandtab
 */

#include "paging.h"
#include "lib.h"
#include "GUI/bga.h"

void paging_init(){
    
    memset(page_table, 0, sizeof(PTE_t) * DIR_TBL_SIZE);
    memset(page_directory, 0, sizeof(PDE_t) * DIR_TBL_SIZE);

    // Initialize the page table.
    int i;
    for (i = 0; i < PAGE_TBL_SIZE; i++) {
        page_table[i].P    = 0;
        page_table[i].RW   = 1;
        page_table[i].US   = 0;
        page_table[i].PWT  = 0;
        page_table[i].PCD  = 0;
        page_table[i].A    = 0;
        page_table[i].D    = 0;
        page_table[i].PAT  = 0;
        page_table[i].G    = 0;
        page_table[i].AVL  = 0;
        page_table[i].ADDR = 0;
    }
    for (i = 0; i < PAGE_TBL_SIZE; i++) {
        vidmap_table[i].P    = 0;
        vidmap_table[i].RW   = 1;
        vidmap_table[i].US   = 0;
        vidmap_table[i].PWT  = 0;
        vidmap_table[i].PCD  = 0;
        vidmap_table[i].A    = 0;
        vidmap_table[i].D    = 0;
        vidmap_table[i].PAT  = 0;
        vidmap_table[i].G    = 0;
        vidmap_table[i].AVL  = 0;
        vidmap_table[i].ADDR = 0;
    }

    // Set four video memory pages (1 for display, 3 for backup).
    page_table[VID_MEM_POS].P    = 1;
    page_table[VID_MEM_POS].ADDR = VID_MEM_POS;
    page_table[VID_MEM_POS + 1].P = 1;
    page_table[VID_MEM_POS + 1].ADDR = VID_MEM_POS + 1;
    page_table[VID_MEM_POS + 2].P = 1;
    page_table[VID_MEM_POS + 2].ADDR = VID_MEM_POS + 2;
    page_table[VID_MEM_POS + 3].P = 1;
    page_table[VID_MEM_POS + 3].ADDR = VID_MEM_POS + 3;

    // Initialize page directories
    for (i = 0; i < DIR_TBL_SIZE; i++) {
        page_directory[i].P    = 0;
        page_directory[i].RW   = 1;
        page_directory[i].US   = 0;
        page_directory[i].PWT  = 0;
        page_directory[i].PCD  = 0;
        page_directory[i].A    = 0;
        page_directory[i].D    = 0;
        page_directory[i].PS   = 0;
        page_directory[i].G    = 0;
        page_directory[i].AVL  = 0;
        page_directory[i].ADDR = 0;
    }

    // Initialize the 4MB page directory for kernel.
    page_directory[1].P    = 1;
    page_directory[1].PS   = 1; // 4MB page
    page_directory[1].ADDR = KERNEL_ADDR >> 12;

    // Initialize the page directory for 4kB page tables.
    page_directory[0].P    = 1;
    page_directory[0].ADDR = (uint32_t)page_table >> 12;

    // Set a page for GUI
    uint32_t vbe_index = QEMU_BASE_ADDR >> 22;
    page_directory[vbe_index].P = 1;
    page_directory[vbe_index].PS = 1;
    page_directory[vbe_index].ADDR = QEMU_BASE_ADDR >> 12;

    // Code for manipulating control registers to enable paging.
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
