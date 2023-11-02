#ifndef _VT_H
#define _VT_H
#include "keyboard.h"
#include "../lib.h"
#include "../filesys.h"

#define INPUT_BUF_SIZE 128

extern void vt_init();
extern int32_t vt_open(const uint8_t* id);
extern int32_t vt_close(int32_t id);
extern int32_t vt_read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t vt_write(int32_t fd, const void* buf, int32_t nbytes);
extern void vt_keyboard(keycode_t keycode, int release);
void vt_putc(char c);

extern operation_table_t stdin_operation_table;
extern operation_table_t stdout_operation_table;

#endif /* _VT_H */ 
