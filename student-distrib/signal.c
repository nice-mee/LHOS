/* filesys.c - implement the read-only file system
 * vim:ts=4 noexpandtab
 */

#include "syscall_task.h"
#include "scheduler.h"
#include "devices/vt.h"
#include "signal.h"
#include "lib.h"

/* __signal_ignore - do nothing, ignore the signal
 * Inputs: None
 * Outputs: None
 * Return: None
 */
void __signal_ignore(void){
    /* do nothing here */
    return;
}

/* __signal_kill_task - write the file
 * Inputs: fd - the file associated with file descriptor to be wrote
           buf - the buffer hold the writing content
           nbytes - the length of bytes to be wrote
 * Outputs: None
 * Return:  0 as asume always success
 */
void __signal_kill_task(void){
    // clear();
    __syscall_halt(0);
    return;
}

/* send_signal - send the signal to the task when certain event occurs
 * Inputs: signum - the signal number of that type of signal
 * Outputs: None
 * Return:  0 if send successfully
 *          -1 if send fails
 */
void send_signal(int32_t signum){
    pcb_t* cur_pcb = get_current_pcb();
    /* if signum is invalid or get_current_pcb fails, send fails */
    if(signum < 0 || signum > 4 || cur_pcb == NULL) return;

    cli();
    cur_pcb->signals[signum].sa_activate = SIG_ACTIVATED;
    sti();
    return;
}

void send_signal_by_pid(int32_t signum, int32_t pid){
    pcb_t* cur_pcb = get_pcb_by_pid(pid);
    /* if signum is invalid or get_current_pcb fails, send fails */
    if(signum < 0 || signum > 4 || cur_pcb == NULL) return;

    cli();
    cur_pcb->signals[signum].sa_activate = SIG_ACTIVATED;
    sti();
    return;
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
    uint32_t ebp0;
    uint32_t user_esp;
    HW_Context_t* context;
    uint32_t execute_sigreturn_size = EXECUTE_SIGRETURN_END - EXECUTE_SIGRETURN;
    uint32_t ret_addr;
    asm ("movl %%ebp, %0" : "=r" (ebp0));
    /* checking pending signals */
    for(i = 0; i < SIG_NUM; i++){
        if(cur_pcb->signals[i].sa_activate == SIG_ACTIVATED && cur_pcb->signals[i].sa_masked == SIG_UNMASK){
            cur_pcb->signals[i].sa_activate = SIG_UNACTIVATED;
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

    /* if handler is in kernel, directly call it and return */
    if(signal_handler == __signal_ignore || signal_handler == __signal_kill_task){
        ((void(*)())signal_handler)();
        for(i = 0; i < SIG_NUM; i++){
            cur_pcb->signals[i].sa_masked = SIG_UNMASK;
        }
        return;
    }

    if (signum == SIGNUM_ALARM) {
        ((void(*)())signal_handler)();
        for(i = 0; i < SIG_NUM; i++){
            cur_pcb->signals[i].sa_masked = SIG_UNMASK;
        }
        return;
    }

    /* then set up the signal handler's stack frame */
    /* first push the execute sigreturn */
    context = (HW_Context_t*)(ebp0 + 8);
    user_esp = context->esp;
    if(user_esp < _128_MB) return;
    ret_addr = user_esp - execute_sigreturn_size;               // return to the execute sigreturn
    memcpy((void*)(ret_addr), EXECUTE_SIGRETURN, execute_sigreturn_size);
    /* then push the hardware context */
    memcpy((void*)(ret_addr - sizeof(HW_Context_t)), context, sizeof(HW_Context_t));
    /* then push the signal number and return address */
    memcpy((void*)(ret_addr - sizeof(HW_Context_t) - 4), &signum, 4);
    memcpy((void*)(ret_addr - sizeof(HW_Context_t) - 8), &ret_addr, 4);

    /* update the hardware context for iret */
    user_esp = ret_addr - sizeof(HW_Context_t) - 8;
    context->esp = user_esp;
    context->ret_addr = (uint32_t)signal_handler;

    return;
}
