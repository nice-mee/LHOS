#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "filesys.h"
#include "pcb.h"
#include "devices/rtc.h"
//#include <string.h>

#define FILE_NAME_LEN 32  // 32B to store file name in FS
#define ARG_LEN     128
#define MAX_ARG_NUM 24
#define INVALID_CMD -1

// Executable check
#define MAGIC_NUMBERS_NUM 4 // the first 4 bytes of the file represent the magic number
#define MAGIC_NUM_1 0x7f
#define MAGIC_NUM_2 0x45
#define MAGIC_NUM_3 0x4c
#define MAGIC_NUM_4 0x46

operation_table_t rtc_operation_table = {
    .open_operation = RTC_open,
    .close_operation = RTC_close,
    .read_operation = RTC_read,
    .write_operation = RTC_write
};

operation_table_t dir_operation_table = {
    .open_operation = dir_open,
    .close_operation = dir_close,
    .read_operation = dir_read,
    .write_operation = dir_write
};

operation_table_t file_operation_table = {
    .open_operation = fopen,
    .close_operation = fclose,
    .read_operation = fread,
    .write_operation = fwrite
};

int32_t __syscall_execute(const uint8_t* command);
int32_t __syscall_halt(uint8_t status);

int32_t __syscall_open(const uint8_t* filename);
int32_t __syscall_close(int32_t fd);
int32_t __syscall_read(int32_t fd, void* buf, int32_t nbytes);
int32_t __syscall_write(int32_t fd, const void* buf, int32_t nbytes);

/*
 *       / \__
 *      (    @\___
 *      /         O
 *     /   (_____/
 *    /_____/   U
 */

#endif
