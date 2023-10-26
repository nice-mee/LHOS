#include "syscall_task.h"
#include "paging.h"
#include "vt.h"
#include "x86_desc.h"

static void set_user_PDE(uint32_t pid)
{
    int32_t PDE_index = _128_MB >> 22;
    page_directory[PDE_index].P    = 1;
    page_directory[PDE_index].US   = 1;
    page_directory[PDE_index].ADDR = (EIGHT_MB + pid * FOUR_MB) >> 12;
}

#define FILE_NAME_LEN 32  // 32B to store file name in FS
#define ARG_LEN     128
#define MAX_ARG_NUM 24
#define INVALID_CMD -1

static int32_t parse_args(const uint8_t* command, uint8_t* args)
{
    if (command == NULL || args == NULL) {
        return INVALID_CMD;
    }

    int i = 0;

    // Copy arguments from command to args
    while (command[i] != '\0') {
        args[i] = command[i];
        i++;
    }
    /*
    if (i >= ARG_LEN) {
        return INVALID_CMD;
    }*/

    // Null-terminate the args
    args[i] = '\0';

    return 0;  // return success
}

static int32_t executable_check(const uint8_t* name)
{

}

static int32_t program_loader(uint32_t inode_index, uint32_t* program_entry_point)
{
    if (-1 == read_data(inode_index, 0, (uint8_t *)EXECUTABLE_START, USER_STACK_START - EXECUTABLE_START)) // This is a little tricky, USER_STACK_START is the end of 4MB page, so we can use it as the end of the loaded program
        return -1;
    *program_entry_point = ((uint32_t *)EXECUTABLE_START)[24]; // 24 is the offset of the entry point in the header
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

    // Buffer to hold the extracted file name
    uint8_t filename[FILE_NAME_LEN + 1];
    
    // Buffer to hold the arguments to the program
    uint8_t args[MAX_COMMAND_LEN + 1];

    // Extract the program name as the first word in the command line
    int i = 0;
    while (command[i] != ' ' && command[i] != '\0') {
        filename[i] = command[i];
        i++;
    }

    // Null-terminate the filename
    filename[i] = '\0';

    // skip the spaces before the args
    while(command[i] == ' ') {
        // Skip the space character before the arguments start
        i++;
    }
    if(command[i] == '\0') {
        return INVALID_CMD;
    }

    // pass the pointer to the first argument to the helper function.
    int32_t flg = parse_args(&command[i], args);
    if (flg) {  // should return 0 on success
        return INVALID_CMD;
    }

    // Executable check

    // set a new dentry for the file
    dentry_t *cur_dentry;
    
    // find the dentry for the file according to its name
    if (-1 == read_dentry_by_name(filename, cur_dentry)) {
        return INVALID_CMD; // the filename is invalid
    }

    int8_t magic_num_buf[MAGIC_NUMBERS_NUM];
    if (-1 == read_data(cur_dentry->inode_index, 0, magic_num_buf, MAGIC_NUMBERS_NUM)) {
        return INVALID_CMD; // read data fail
    }

    if(magic_num_buf[0] != MAGIC_NUM_1 || magic_num_buf[1] != MAGIC_NUM_2 ||
       magic_num_buf[2] != MAGIC_NUM_3 || magic_num_buf[3] != MAGIC_NUM_4) {
        return INVALID_CMD; // file not executable
    }

    // Set up program paging
    cli();
    int pid = get_available_pid();
    if (pid == -1) {
        return INVALID_CMD; // no available pid
    }
    set_user_PDE(pid);

    // User-level Program Loader
    uint32_t program_entry_point;
    if (-1 == program_loader(cur_dentry->inode_index, &program_entry_point)) {
        return INVALID_CMD; // program loader fail
    }
    
    // Create PCB
    pcb_t* parent_pcb = get_current_pcb();
    pcb_t* cur_pcb = create_pcb(pid, parent_pcb);
    
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
        cur_pcb->fd_array[i].flags = 0;
        cur_pcb->fd_array[i].operation_table->close_operation(i);
    }

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
