/* exception_handler.h - Header for exception_handler.c
 * vim:ts=4 noexpandtab
 */
#include "lib.h"
#include "syscall_task.h"
#include "signal.h"

#define GENERATE_EXCEPTION_HANDLER(idtvec, str, name) \
extern void __##name() \
{ \
    printf("Exception 0x%x: " str "\n" , idtvec); \
    send_signal(SIGNUM_SEGFAULT); \
}
