#include "pcb.h"
#include "paging.h"
#include <string.h>

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

static int32_t setup_paging()
{

}

static int32_t program_loader()
{

}

static int32_t create_pcb()
{

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
    
    
    // Set up program paging

    // User-level Program Loader
    
    // Create PCB
    
    // Context Switch

    return 0;
}



/* __syscall_halt - halt the current process
 * Inputs: status - the given status to be returned to parent process
 * Outputs: the specified value in status
 * Side Effects: This call should never return to the caller
 */
int32_t __syscall_halt(uint8_t status) {
    pcb_t* cur_pcb = get_current_pcb();
    pcb_t* parent_pcb = cur_pcb->parent_pcb;

    // Restore parent data
    

    // Restore parent paging
    set_user_PDE(parent_pcb->pid);

    // @todo: Close all FDs

    
    // Context Switch
    int32_t ret = (int32_t)status;
    asm volatile("movl %0, %%esp;"
                 "movl %1, %%ebp;"
                 "movl %2, %%eax;"
                 "leave;"
                 "ret;"
                :: "r"(parent_pcb->esp),
                   "r"(parent_pcb->ebp), 
                   "r"(ret)
                : "eax", "esp", "ebp"
    );
    return 0; // This return serves no purpose other than to silence the compiler
}
