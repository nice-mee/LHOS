/* idt.c - Defines for helper functions for initializing the IDT
 * vim:ts=4 noexpandtab
 */
#ifndef _IDT_H
#define _IDT_H
#include "x86_desc.h"
#include "idtentry.h"

#define SYSCALL_VEC 0x80
#define KEYBOARD_VEC 0x21
#define RTC_VEC 0x28
#define PIT_VEC 0x20

extern void idt_init();
extern void temp_syscall_handler();

#endif /* _IDT_H */
