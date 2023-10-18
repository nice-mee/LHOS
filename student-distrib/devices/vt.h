#ifndef _VT_H
#define _VT_H
#include "keyboard.h"
#include "../lib.h"

extern void vt_init();
extern void vt_open();
extern void vt_close();
extern int32_t vt_read(void* buf, int32_t nbytes);
extern int32_t vt_write(const void* buf, int32_t nbytes);
extern void vt_keyboard(uint8_t keycode, int release);
void vt_putc(uint8_t c);


#endif /* _VT_H */ 
