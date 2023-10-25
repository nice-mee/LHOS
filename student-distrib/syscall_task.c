#include "pcb.h"

static int32_t parse_args(const uint8_t* command, uint8_t* args)
{

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
    // Restore parent data

    // Restore parent paging
    
    // Close all FDs
    
    // Context Switch

    return 0;
}
