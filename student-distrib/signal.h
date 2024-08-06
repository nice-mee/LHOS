/* filesys.h - Defines the read-only file system
 * vim:ts=4 noexpandtab
 */

#ifndef _SIGNAL_H
#define _SIGNAL_H

#include "types.h"

/* 
 *      define basic constant for signals table
 *      +---------------+---------------+-------------------+
 *      | Signal Name   | Signal Number | Default Action    |
 *      +---------------+---------------+-------------------+
 *      | DIV_ZERO      | 0             | Kill the task     |
 *      +---------------+---------------+-------------------+
 *      | SEGFAULT      | 1             | Kill the task     |
 *      +---------------+---------------+-------------------+
 *      | INTERRUPT     | 2             | Kill the task     |
 *      +---------------+---------------+-------------------+
 *      | ALARM         | 3             | Ignore            |
 *      +---------------+---------------+-------------------+
 *      | USER1         | 4             | Ignore            |
 *      +---------------+---------------+-------------------+
*/
#define SIG_NUM 5
#define SIGNUM_DIV_ZERO 0
#define SIGNUM_SEGFAULT 1
#define SIGNUM_INTERRUPT 2
#define SIGNUM_ALARM 3
#define SIGNUM_USER1 4
#define SIG_ACTIVATED 1
#define SIG_UNACTIVATED 0
#define SIG_MASK 1
#define SIG_UNMASK 0

/* define signal structure */
typedef struct signal{
    int32_t sa_activate;            // 0 as unactivated, 1 as activated
    int32_t sa_masked;              // 0 as unmasked, 1 as masked
    void* sa_handler;
} signal_t;

/* define Hardware context structure */
typedef struct HW_Context{
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t eax;
    uint16_t ds;
    uint16_t padding1;
    uint16_t es;
    uint16_t padding2;
    uint16_t fs;
    uint16_t padding3;
    uint32_t error_Code;
    uint32_t ret_addr;
    uint16_t cs;
    uint16_t padding4;
    uint32_t eflags;
    uint32_t esp;
    uint16_t ss;
    uint16_t padding5;
} HW_Context_t;

/* functions used by signals system */
void __signal_ignore(void);
void __signal_kill_task(void);
void send_signal(int32_t signum);
void send_signal_by_pid(int32_t signum, int32_t pid);
void handle_signal(void);
void EXECUTE_SIGRETURN(void);
void EXECUTE_SIGRETURN_END(void);

#endif /* _SIGNALS_H */
