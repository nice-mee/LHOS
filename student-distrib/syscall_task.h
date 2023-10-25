#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "filesys.h"
#include "pcb.h"
#include <string.h>

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

static int32_t parse_args(const uint8_t* command, uint8_t* args);
int32_t __syscall_execute(const uint8_t* command);



#endif