#include "rtc.h"
#include "../lib.h"
#include "../i8259.h"
#include "../filesys.h"
#include "../pcb.h"

/* struct storing frequency and counter for process with
different frequencies. Used to implement virtualization */
typedef struct {
    int32_t proc_exist;
    int32_t proc_freq;
    volatile int32_t proc_count;
} proc_freqcount_pair;

static proc_freqcount_pair RTC_proc_list[MAX_PROC_NUM];

operation_table_t RTC_operation_table = {
    .open_operation = RTC_open,
    .close_operation = RTC_close,
    .read_operation = RTC_read,
    .write_operation = RTC_write
};

/* RTC_init - Initialization of Real-Time Clock (RTC)
 * 
 * Initializes the RTC to a base frequency of 1024.
 *  
 * Inputs: none
 * Outputs: none (Configuration of RTC registers)
 * Side Effects: Modifies RTC control registers A and B. Resets the rtc_time_counter and enables
 *               the RTC interrupt request line (IRQ).
 */
void RTC_init(void) {
    printf("RTC_init\n");
    char prev;
    outb(RTC_B, RTC_PORT);     // select register B, and disable NMI
    prev = inb(RTC_CMOS_PORT); // read the current value of register B
    outb(RTC_B, RTC_PORT);     // set the index again (inb will reset the idx)
    /* write the previous value ORed with 0x40. This turns on bit 6 of register B */
    outb(prev | 0x40, RTC_CMOS_PORT);

    outb(RTC_A, RTC_PORT);     // set index to register A, disable NMI
    prev = inb(RTC_CMOS_PORT); // get the previous value of register B
    outb(RTC_A, RTC_PORT);     // set the index again
    outb((prev & 0xF0) | RTC_BASE_RATE, RTC_CMOS_PORT);  // set the frequency to 2 Hz

    enable_irq(RTC_IRQ);
}


/* __intr_RTC_handler - Real-Time Clock (RTC) Interrupt Handler
 * 
 * Handles interrupts generated by the RTC.
 * 
 * Inputs: None (Triggered by RTC interrupt)
 * Outputs: None (Handles interrupt side effects)
 * Side Effects: Increments rtc_time_counter. Acknowledges RTC interrupt by reading from register C.
 *               Sends EOI to the RTC IRQ. Potentially triggers other interrupt handlers through the
 *               test_interrupts() function.
 */
void __intr_RTC_handler(void) {
    cli();
    send_eoi(RTC_IRQ);
    int32_t pid;
    /* update each process's counter */
    for (pid = 0; pid < MAX_PROC_NUM; ++pid) {
            RTC_proc_list[pid].proc_count --;
    }
    outb(RTC_C &0x0F, RTC_PORT); // select register C
    inb(RTC_CMOS_PORT);		    // just throw away contents

    sti();
}

/* 
 * Function: RTC_open
 * Description: Initializes the RTC interrupt handler for a specific process, 
 * setting the RTC frequency to 2Hz and validating the process's ID if the fd_array
 * has empty space
 * Parameters:
 *    fd: meaningless here
 * Returns: file descriptor for success, -1 for failure
 * Side Effects: 1. sets the RTC frequency for the process to 2Hz
 *               2. validates the process's id
 */
int32_t RTC_open(const uint8_t* fd) {
    int32_t i;
    pcb_t* cur_pcb = get_current_pcb();
    file_descriptor_t* cur_fd;
    for(i = 2; i < NUM_FILES; i++){             // 2 as stdin and stdout already been used
        cur_fd = &(cur_pcb->fd_array[i]);
        if(cur_fd->flags == READY_TO_BE_USED){
            /* if there exists empty file descriptor, assign it */
            cur_fd->operation_table = &RTC_operation_table;
            cur_fd->inode_index = 0;
            cur_fd->file_position = 0;
            cur_fd->flags = IN_USE;
            /* set process's freq and existence status */
            RTC_proc_list[i].proc_freq = 2;
            RTC_proc_list[i].proc_count = RTC_BASE_FREQ / 2; 
            RTC_proc_list[i].proc_exist = 1;
            return i;
        }
    }
    return -1;  // if no empty, open fail
}

/* 
 * Function: RTC_close
 * Description: invalidates the process's RTC by 
 * marking its id as invalid.
 * Parameters:
 *    fd: file descriptor
 * Returns: 0 for success, -1 for failure
 * Side Effects: set the process's id invalid
 */
int32_t RTC_close(int32_t fd) {
    pcb_t* cur_pcb = get_current_pcb();
    file_descriptor_t* cur_fd;
    /* if if out of boundary, close fail */
    if(fd < 2 || fd >= NUM_FILES) return -1;

    cur_fd = &(cur_pcb->fd_array[fd]);
    /* if that id is invalid, close fail */
    if(cur_fd->flags != IN_USE || cur_fd->inode_index != 0) return -1;

    /* free that file descriptor if every thing all right */
    cur_fd->flags = READY_TO_BE_USED;
    /* invalidate the process's existence status */
    RTC_proc_list[cur_pcb->pid].proc_exist = 0;
    return 0;
}

/* 
 * Function: RTC_read
 * Description: block until the next interrupt occurs for the process
 * Parameters:
 *    fd: file descriptor
 *    buf: ignored
 *    nbytes: ignored
 * Returns:
 *    0 for success
 * Side Effects: block until the next interrupt occurs for the process
 */
int32_t RTC_read(int32_t fd, void* buf, int32_t nbytes) {
    /* if proc_id out of boundary, read fails */
    if(fd < 2 || fd >= MAX_PROC_NUM) return -1;

    /* virtualization: wait counter reaches zero */
    int32_t proc_id = get_current_pid();
    while(RTC_proc_list[proc_id].proc_count > 0);
    /* reset counter */
    cli();
    if(RTC_proc_list[proc_id].proc_freq)
        RTC_proc_list[proc_id].proc_count = RTC_BASE_FREQ / RTC_proc_list[proc_id].proc_freq;
    sti();
    return 0;
}

/* 
 * Function: RTC_write
 * Description: adjusts the RTC frequency for a specific process
 * Parameters:
 *    fd: file descriptor
 *    buf: ptr to the value of the intended frequency
 *    nbytes: ignored
 * Returns:
 *    0 for success, -1 for invalid arguments
 * Side Effects: adjusts the freq for the process
 */
int32_t RTC_write(int32_t fd, const void* buf, int32_t nbytes) {
    int32_t freq;
    /* if buf is NULL or proc_id out of boundary, write fails */
    if(fd < 2 || fd >= MAX_PROC_NUM || buf == NULL) return -1;
    
    freq = *(int32_t*) buf;
    if(freq <= 0) return -1;
    /* ensuring freq is a power of 2 and within acceptable limits */
    if(!(freq && !(freq & (freq - 1))) || freq > 1024) {
        return -1;
    }
    /* adjusts the freq */
    int32_t proc_id = get_current_pid();
    cli();
    RTC_proc_list[proc_id].proc_freq = freq;
    RTC_proc_list[proc_id].proc_count = RTC_BASE_FREQ / RTC_proc_list[proc_id].proc_freq;
    sti();
    return 0;
}
