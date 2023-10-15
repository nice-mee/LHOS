#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEYBOARD_IRQ 1

extern void __intr_keyboard_handler();
extern void keyboard_init();

#endif /* KEYBOARD_H */
