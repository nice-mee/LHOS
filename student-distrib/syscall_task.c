#include "syscall_task.h"
#include "paging.h"
#include "devices/vt.h"
#include "x86_desc.h"

static void set_user_PDE(uint32_t pid)
{
    int32_t PDE_index = _128_MB >> 22;
    page_directory[PDE_index].P    = 1;
    page_directory[PDE_index].PS   = 1;
    page_directory[PDE_index].US   = 1;
    page_directory[PDE_index].ADDR = (EIGHT_MB + pid * FOUR_MB) >> 12;

    // flushing TLB by reloading CR3 register
    asm volatile (
        "movl %%cr3, %%eax;"  // Move the value of CR3 into EBX
        "movl %%eax, %%cr3;"  // Move the value from EBX back to CR3
        : : : "eax", "memory"
);


}

/**
 * parse_args
 * parse a command line input
 *
 * Inputs:
 * - command: ptr to a string (uint8_t) representing the command input
 * - filename: ptr to the file name
 * - args: ptr to args buffer
 *
 * Output:
 * - Returns 0 upon successful parsing.
 * - Returns -1 for nullptr
 */
static int32_t parse_args(const uint8_t* command, uint8_t* filename, uint8_t* args)
{
    // Validate inputs
    if (command == NULL || filename == NULL || args == NULL) {
        return INVALID_CMD;
    }

    int i = 0, j = 0, name_len = 0;
    // go through spaces before the filename
    while (command[i] == ' ') i++;

    // extracting the filename
    while (command[i] != ' ' && command[i] != '\0') {
        filename[j] = command[i];
        i++;
        j++;
        name_len++;
    }
    filename[j] = '\0';
    
    // empty name or longer than maxi
    if(!name_len || name_len > FILE_NAME_LEN) {
        return INVALID_CMD;
    }

    // skip the spaces between the filename and the arguments
    while (command[i] == ' ') i++;

    // If there are no arguments in the command, ensure the arguments string is empty
    if (command[i] == '\0') {
        args[0] = '\0';  // Ensure the argument string is empty
        return 0;
    }

    // copy the rest of the command into args buffer
    j = 0;  // reset index for args array
    while (command[i] != '\0') {
        args[j] = command[i];
        i++;
        j++;
    }
    args[j] = '\0';

    return 0;  // Indicate successful parsing
}


static int32_t executable_check(const uint8_t* name)
{
     // set a new dentry for the file
    dentry_t cur_dentry;
    
    // find the dentry for the file according to its name
    if (-1 == read_dentry_by_name(name, &cur_dentry)) {
        return INVALID_CMD; // the filename is invalid
    }

    uint8_t magic_num_buf[MAGIC_NUMBERS_NUM];
    if (-1 == read_data(cur_dentry.inode_index, 0, magic_num_buf, MAGIC_NUMBERS_NUM)) {
        return INVALID_CMD; // read data fail
    }

    if(magic_num_buf[0] != MAGIC_NUM_1 || magic_num_buf[1] != MAGIC_NUM_2 ||
       magic_num_buf[2] != MAGIC_NUM_3 || magic_num_buf[3] != MAGIC_NUM_4) {
        return INVALID_CMD; // file not executable
    }

    return 0; // file executable
}

static int32_t program_loader(uint32_t inode_index, uint32_t* program_entry_point)
{
    if (-1 == read_data(inode_index, 0, (uint8_t *)EXECUTABLE_START, USER_STACK_START - EXECUTABLE_START)) // This is a little tricky, USER_STACK_START is the end of 4MB page, so we can use it as the end of the loaded program
        return -1;
    *program_entry_point = *((uint32_t *)(EXECUTABLE_START + 24)); // 24 is the offset of the entry point in the header
    return 0;
}

static pcb_t* create_pcb(uint32_t pid, pcb_t *parent_pcb)
{
    pcb_t *pcb = get_pcb_by_pid(pid);
    memset(pcb, 0, sizeof(pcb_t)); // initialize the pcb to all 0
    pcb->pid = pid;
    pcb->parent_pcb = parent_pcb;

    /* Set up FDs */
    // stdin
    pcb->fd_array[0] = (file_descriptor_t) {
        .operation_table = &stdin_operation_table,
        .inode_index = 0,
        .file_position = 0,
        .flags = 1
    };
    // stdout
    pcb->fd_array[1] = (file_descriptor_t) {
        .operation_table = &stdout_operation_table,
        .inode_index = 0,
        .file_position = 0,
        .flags = 1
    };

    return pcb;
}

/* __syscall_execute - execute the given command
 * Inputs: command - the given command to be executed
 * Outputs: -1 if the command cannot be executed,
 *          256 if the program dies by an exception
 *          0-255 if the program executes a halt system call
 * Side Effects: None
 */
int32_t __syscall_execute(const uint8_t* command) {
    // Parse args
    if (command == NULL) {
        return INVALID_CMD;
    }

    uint8_t filename[FILE_NAME_LEN + 1];  // store  the file name
    uint8_t args[ARG_LEN + 1];  // store args

    if (parse_args(command, filename, args)) {  // return value should be 0 upon success
        return INVALID_CMD;
    }

    // Executable check
    if (executable_check(filename)) {
        return INVALID_CMD;
    }

    // Set up program paging
    cli();
    int pid = get_available_pid();
    if (pid == -1) {
        return INVALID_CMD; // no available pid
    }
    set_user_PDE(pid);

    // User-level Program Loader
    dentry_t cur_dentry;
    read_dentry_by_name(filename, &cur_dentry);
    uint32_t program_entry_point;
    if (-1 == program_loader(cur_dentry.inode_index, &program_entry_point)) {
        return INVALID_CMD; // program loader fail
    }
    
    // Create PCB
    pcb_t* parent_pcb = get_current_pcb();
    pcb_t* cur_pcb = create_pcb(pid, parent_pcb);
    
    // set TSS
    tss.ss0 = KERNEL_DS;
    tss.esp0 = EIGHT_MB - cur_pcb->pid * EIGHT_KB;

    asm volatile("movl %%esp, %0;"
                 "movl %%ebp, %1;"
                : "=r"(parent_pcb->esp),
                  "=r"(parent_pcb->ebp)
                : : "memory");

    sti();
    // Context Switch
    asm volatile("pushl %0;"
                 "pushl %1;"
                 "pushfl;"
                 "pushl %2;"
                 "pushl %3;"
                 "iret;"
                :: "r"(USER_DS), // we don't have SS, so we use DS instead
                   "r"(USER_STACK_START),
                   "r"(USER_CS),
                   "r"(program_entry_point)
                : "memory");

    return 0; // This return serves no purpose other than to silence the compiler
}



/* __syscall_halt - halt the current process
 * Inputs: status - the given status to be returned to parent process
 * Outputs: the specified value in status
 * Side Effects: This call should never return to the caller
 */
int32_t __syscall_halt(uint8_t status) {
    // Restore parent data
    pcb_t* cur_pcb = get_current_pcb();
    pcb_t* parent_pcb = cur_pcb->parent_pcb;

    // Restore parent paging
    set_user_PDE(parent_pcb->pid);

    // Close all FDs
    int i;
    for (i = 0; i < NUM_FILES; i++) {
        if (cur_pcb->fd_array[i].flags == 0) continue;
        cur_pcb->fd_array[i].flags = 0;
        cur_pcb->fd_array[i].operation_table->close_operation(i);
    }

    // Write Parent process's info back to TSS
    tss.ss0 = KERNEL_DS;
    tss.esp0 = EIGHT_MB - parent_pcb->pid * EIGHT_KB;

    free_pid(cur_pcb->pid);

    // Context Switch
    int32_t ret = (int32_t)status;
    asm volatile("movl %0, %%esp;"
                 "movl %1, %%ebp;"
                 "movl %2, %%eax;"
                 // Stack pointer and base pointer are restored from __syscall_execute in parent PCB
                 // It's like resuming __syscall_execute but with different eip
                 "leave;"
                 "ret;"
                :: "r"(parent_pcb->esp),
                   "r"(parent_pcb->ebp), 
                   "r"(ret)
                : "eax", "esp", "ebp"
    );
    return 0; // This return serves no purpose other than to silence the compiler
}

/* __syscall_open - open the file
 * Inputs: filename - the name of the file to be opened
 * Outputs: None
 * Return: file descriptor if successfully
 *        -1 if open fails
 * Side Effects: This call should never return to the caller
 */
int32_t __syscall_open(const uint8_t* filename){
    dentry_t cur_dentry;
    // find the dentry for the file according to its name
    // if the file does not exist, open fails
    if(0 != read_dentry_by_name(filename, &cur_dentry)) return -1;
    if(cur_dentry.file_type == RTC_FILE_TYPE)
        return RTC_open(filename);
    else if(cur_dentry.file_type == DIR_FILE_TYPE)
        return dir_open(filename);
    else if(cur_dentry.file_type == REGULAR_FILE_TYPE)
        return fopen(filename);
    else{
        return -1;
    }
    return 0;
}

/* __syscall_close - close the file
 * Inputs: fd - the file associated with file descriptor to be closed
 * Outputs: None
 * Return: 0 if successfully, -1 if close fails
 * Side Effects: This call should never return to the caller
 */
int32_t __syscall_close(int32_t fd){
    // if fd out of boundary or fd is stdin or stdout, close fails
    if(fd >= NUM_FILES || fd < 2)
        return -1;
    pcb_t* cur_pcb = get_current_pcb();
    return cur_pcb->fd_array[fd].operation_table->close_operation(fd);
}

/* __syscall_read - read the file
 * Inputs: fd - the file associated with file descriptor to be read
           buf - the buffer to load the reading content
           nbytes - the length of bytes to be read
 * Outputs: None
 * Return:  number of bytes read if successfully
 *          0 if file reach the end
 *          -1 if read fails
 * Side Effects: This call should never return to the caller
 */
int32_t __syscall_read(int32_t fd, void* buf, int32_t nbytes){
    /* input being checked in read operation */
    /* increment of file_position is handled in read_operation */
    return get_current_pcb()->fd_array[fd].operation_table->read_operation(fd, buf, nbytes);
}

/* __syscall_read - write the file
 * Inputs: fd - the file associated with file descriptor to be wrote
           buf - the buffer hold the writing content
           nbytes - the length of bytes to be wrote
 * Outputs: None
 * Return:  number of bytes read if successfully
 *          0 if file reach the end
 *          -1 if read fails
 * Side Effects: This call should never return to the caller
 */
int32_t __syscall_write(int32_t fd, const void* buf, int32_t nbytes){
    pcb_t* cur_pcb = get_current_pcb();
    /* if fd out of boundary or buf or nbytes is incalid, read fails */
    if(fd < 0 || fd >= NUM_FILES || buf == NULL || nbytes < 0) return -1;

    /* increment of file_position is handled in write_operation */
    return cur_pcb->fd_array[fd].operation_table->write_operation(fd, buf, nbytes);
}

int32_t __syscall_getargs(int32_t fd){
    return 0;
}

int32_t __syscall_vidmap(int32_t fd){
    return 0;
}

int32_t __syscall_set_handler(int32_t fd){
    return 0;
}

int32_t __syscall_sigreturn(int32_t fd){
    return 0;
}
