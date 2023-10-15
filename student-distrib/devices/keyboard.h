#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "device_utils.h"

#define KEYBOARD_IRQ 1
#define KEYBOARD_PORT 0x60

#define SCANCODE_LSHIFT_PRESSED 0x2A
#define SCANCODE_LSHIFT_RELEASED 0xAA
#define SCANCODE_RSHIFT_PRESSED 0x36
#define SCANCODE_RSHIFT_RELEASED 0xB6

typedef struct {
    int shift;
    int caps;
    int ctrl;
    int alt;
} keyboard_state_t;

DECLARE_DEVICE_HANDLER(keyboard);
DECLARE_DEVICE_INIT(keyboard);

#endif /* _KEYBOARD_H */
