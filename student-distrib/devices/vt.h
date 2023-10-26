#ifndef _VT_H
#define _VT_H
#include "keyboard.h"
#include "../lib.h"
#include "../filesys.h"

#define INPUT_BUF_SIZE 128

extern void vt_init();
extern void vt_open();
extern void vt_close();
extern int32_t vt_read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t vt_write(int32_t fd, const void* buf, int32_t nbytes);
extern void vt_keyboard(keycode_t keycode, int release);
void vt_putc(char c);

operation_table_t stdin_operation_table = {
    .open_operation = NULL,
    .close_operation = NULL,
    .read_operation = vt_read,
    .write_operation = NULL
};

operation_table_t stdout_operation_table = {
    .open_operation = NULL,
    .close_operation = NULL,
    .read_operation = NULL,
    .write_operation = vt_write
};

#endif /* _VT_H */ 
