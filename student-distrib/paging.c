/* paging.c - Functions to help set up paging
 * vim:ts=4 noexpandtab
 */

#include "paging.h"
#include "dynamic_alloc.h"
#include "lib.h"
#include "GUI/bga.h"

void paging_init(){
    
    memset(page_table, 0, sizeof(PTE_t) * DIR_TBL_SIZE);
    memset(page_directory, 0, sizeof(PDE_t) * DIR_TBL_SIZE);
    memset(vidmap_table, 0, sizeof(PDE_t) * DIR_TBL_SIZE);
    memset(dynamic_tables, 0, sizeof(PTE_t) * MAX_PID_NUM);

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

    page_table[GUI_VID_MEM_POS].P    = 1;
    page_table[GUI_VID_MEM_POS].ADDR = GUI_VID_MEM_POS;
    page_table[GUI_VID_MEM_POS + 1].P = 1;
    page_table[GUI_VID_MEM_POS + 1].ADDR = GUI_VID_MEM_POS + 1;
    page_table[GUI_VID_MEM_POS + 2].P = 1;
    page_table[GUI_VID_MEM_POS + 2].ADDR = GUI_VID_MEM_POS + 2;
    page_table[GUI_VID_MEM_POS + 3].P = 1;
    page_table[GUI_VID_MEM_POS + 3].ADDR = GUI_VID_MEM_POS + 3;

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

    // Set MAX_PID_NUM dynamic memory page
    page_directory[DYNAMIC_MEMORY_START >> 22].P = 1;
    page_directory[DYNAMIC_MEMORY_START >> 22].US = 1;
    page_directory[DYNAMIC_MEMORY_START >> 22].ADDR = (uint32_t)(dynamic_tables) >> 12;
    for (i = 0; i < PAGE_TBL_SIZE; i++) {
        dynamic_tables[i].P    = 0;
        dynamic_tables[i].RW   = 1;
        dynamic_tables[i].US   = 1;
        dynamic_tables[i].PWT  = 0;
        dynamic_tables[i].PCD  = 0;
        dynamic_tables[i].A    = 0;
        dynamic_tables[i].D    = 0;
        dynamic_tables[i].PAT  = 0;
        dynamic_tables[i].G    = 0;
        dynamic_tables[i].AVL  = 0;
        dynamic_tables[i].ADDR = (DYNAMIC_MEMORY_START + i * PAGE_SIZE) >> 12;
    }

    // Initialize the page directory for 4kB page tables.
    page_directory[0].P    = 1;
    page_directory[0].ADDR = (uint32_t)page_table >> 12;

    // NANI buffer for erow_t
    page_directory[NANI_STATIC_BUF_ADDR >> 22].P = 1;
    page_directory[NANI_STATIC_BUF_ADDR >> 22].US = 1;
    page_directory[NANI_STATIC_BUF_ADDR >> 22].PS = 1;
    page_directory[NANI_STATIC_BUF_ADDR >> 22].ADDR = NANI_STATIC_BUF_ADDR >> 12;
    page_directory[(NANI_STATIC_BUF_ADDR + FOUR_MB) >> 22].P = 1;
    page_directory[(NANI_STATIC_BUF_ADDR + FOUR_MB) >> 22].US = 1;
    page_directory[(NANI_STATIC_BUF_ADDR + FOUR_MB) >> 22].PS = 1;
    page_directory[(NANI_STATIC_BUF_ADDR + FOUR_MB) >> 22].ADDR = (NANI_STATIC_BUF_ADDR + FOUR_MB) >> 12;
    page_directory[(NANI_STATIC_BUF_ADDR + 2 * FOUR_MB) >> 22].P = 1;
    page_directory[(NANI_STATIC_BUF_ADDR + 2 * FOUR_MB) >> 22].US = 1;
    page_directory[(NANI_STATIC_BUF_ADDR + 2 * FOUR_MB) >> 22].PS = 1;
    page_directory[(NANI_STATIC_BUF_ADDR + 2 * FOUR_MB) >> 22].ADDR = (NANI_STATIC_BUF_ADDR + 2 * FOUR_MB) >> 12;
    // NANI buffer for writing file
    page_directory[(NANI_STATIC_BUF_ADDR + 3 * FOUR_MB) >> 22].P = 1;
    page_directory[(NANI_STATIC_BUF_ADDR + 3 * FOUR_MB) >> 22].US = 1;
    page_directory[(NANI_STATIC_BUF_ADDR + 3 * FOUR_MB) >> 22].PS = 1;
    page_directory[(NANI_STATIC_BUF_ADDR + 3 * FOUR_MB) >> 22].ADDR = (NANI_STATIC_BUF_ADDR + 2 * FOUR_MB) >> 12;


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
