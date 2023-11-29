/* filesys.c - implement the read-only file system
 * vim:ts=4 noexpandtab
 */

#include "syscall_task.h"
#include "signal.h"

/* __signal_ignore - do nothing, ignore the signal
 * Inputs: None
 * Outputs: None
 * Return: 0 as asume always success
 */
int32_t __signal_ignore(void){
    /* do nothing here */
    return 0;
}

/* __signal_kill_task - write the file
 * Inputs: fd - the file associated with file descriptor to be wrote
           buf - the buffer hold the writing content
           nbytes - the length of bytes to be wrote
 * Outputs: None
 * Return:  0 as asume always success
 */
int32_t __signal_kill_task(void){
    clear();
    __syscall_halt(0);
    return 0;
}

/* send_signal - send the signal to the task when certain event occurs
 * Inputs: signum - the signal number of that type of signal
 * Outputs: None
 * Return:  0 if send successfully
 *          -1 if send fails
 */
int32_t send_signal(int32_t signum){
    pcb_t* cur_pcb = get_current_pcb();
    /* if signum is invalid or get_current_pcb fails, send fails */
    if(signum < 0 || signum > 4 || cur_pcb == NULL) return -1;

    cli();
    /* if the signal is INTERRUPT, do we need to change the cur_pcb ?????????????? I saw CZY does so */
    cur_pcb->signals[signum].sa_activate = SIG_ACTIVATED;
    sti();
    return 0;
}

/* handle_signal - handle the signal, called everytime when returning to user space, should be called in return-to-user space linkage
 * Inputs: None
 * Outputs: None
 * Return:  None
 */
void handle_signal(void){
    int32_t i, signum = -1;
    pcb_t* cur_pcb = get_current_pcb();
    void* signal_handler;
    uint32_t ebp0 asm("ebp");
    uint32_t user_esp;
    HW_Context_t* context;
    uint32_t execute_sigreturn_size = execute_sigreturn - execute_sigreturn_end;
    uint32_t ret_addr;
    /* checking pending signals */
    for(i = 0; i < SIG_NUM; i++){
        if(cur_pcb->signals[i].sa_activate == SIG_ACTIVATED){
            signum = i;
            signal_handler = cur_pcb->signals[i].sa_handler;
            break;
        }
    }
    /* if no pending signals, return directly */
    if(signum == -1) return;

    /* if exists pending signals, handle it by firstly mask all other signals */
    for(i = 0; i < SIG_NUM; i++){
        if(i != signum) cur_pcb->signals[i].sa_masked = SIG_MASK;
    }

    /* then set up the signal handler's stack frame */
    /* first push the execute sigreturn */
    context = (HW_Context_t*)(ebp0 + 8);
    user_esp = context->esp;
    ret_addr = user_esp - execute_sigreturn_size;               // return to the execute sigreturn
    memcpy((void*)(ret_addr), execute_sigreturn, execute_sigreturn_size);
    /* then push the hardware context */
    memcpy((void*)(ret_addr - sizeof(HW_Context_t)), context, sizeof(HW_Context_t));
    /* then push the signal number and return address */
    memcpy((void*)(ret_addr - sizeof(HW_Context_t) - 4), &signum, 4);
    memcpy((void*)(ret_addr - sizeof(HW_Context_t) - 8), &ret_addr, 4);

    /* update the hardware context for iret */
    context->esp = user_esp;
    context->ret_addr = (uint32_t)signal_handler;

    /* finally execute the signal handler */
    ((void(*)())signal_handler)();

    return;
}
