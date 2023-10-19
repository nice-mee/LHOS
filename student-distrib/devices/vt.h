#ifndef _VT_H
#define _VT_H
#include "keyboard.h"
#include "../lib.h"

extern void vt_init();
extern void vt_open();
extern void vt_close();
extern int32_t vt_read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t vt_write(int32_t fd, const void* buf, int32_t nbytes);
extern void vt_keyboard(keycode_t keycode, int release);
void vt_putc(char c);


#endif /* _VT_H */ 
