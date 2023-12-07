#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "filesys.h"
#include "pcb.h"
#include "devices/rtc.h"
#include "devices/vt.h"
#include "date.h"

#define FILE_NAME_LEN 32  // 32B to store file name in FS
#define MAX_ARG_NUM 24
#define INVALID_CMD -1

// Executable check
#define MAGIC_NUMBERS_NUM 4 // the first 4 bytes of the file represent the magic number
#define MAGIC_NUM_1 0x7f
#define MAGIC_NUM_2 0x45
#define MAGIC_NUM_3 0x4c
#define MAGIC_NUM_4 0x46

int32_t __syscall_execute(const uint8_t* command);
int32_t __syscall_halt(uint8_t status);

int32_t __syscall_open(const uint8_t* filename);
int32_t __syscall_close(int32_t fd);
int32_t __syscall_read(int32_t fd, void* buf, int32_t nbytes);
int32_t __syscall_write(int32_t fd, const void* buf, int32_t nbytes);

int32_t __syscall_getargs(uint8_t* buf, int32_t nbytes);
int32_t __syscall_vidmap(uint8_t** screen_start);
int32_t __syscall_set_handler(int32_t signum, void* handler_address);
int32_t __syscall_sigreturn(void);
int32_t __syscall_ioctl(int32_t fd, int32_t flag);
int32_t __syscall_ps(void);
int32_t __syscall_date(void);
int32_t __syscall_donut(void);

/*
 *       / \__
 *      (    @\___
 *      /         O
 *     /   (_____/
 *    /_____/   U
 */

#endif
