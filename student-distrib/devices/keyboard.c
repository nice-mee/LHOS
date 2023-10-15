#include "keyboard.h"
#include "../lib.h"
#include "../i8259.h"

static keyboard_state_t keyboard_state;

static uint8_t scan_code_mapping[80] =
{
    0, 0,
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    0, 0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    0, 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0,
    '*',
    0,
    ' ',
    0,
};

// static int rtc_state = 0;

DEFINE_DEVICE_HANDLER(keyboard) {
    cli();
    send_eoi(KEYBOARD_IRQ);
    unsigned char scan_code = inb(KEYBOARD_PORT);
    // printf("Keyboard interrupt, scan code: 0x%x\n", scan_code);
    if (scan_code > 80)
        return;
    if (scan_code >= 2 && scan_code <= 0x80) {
        putc(scan_code_mapping[scan_code]);
    }
    // if (scan_code == SCANCODE_LSHIFT_PRESSED) {
    //     keyboard_state.shift = 1;
    //     if (rtc_state == 0) {
    //         rtc_state = 1;
            
    //     } else {
    //         rtc_state = 0;

    //     }
    // }
    sti();
}

DEFINE_DEVICE_INIT(keyboard) {
    printf("Keyboard_init\n");
    keyboard_state.shift = 0;
    keyboard_state.caps = 0;
    keyboard_state.ctrl = 0;
    keyboard_state.alt = 0;
    enable_irq(KEYBOARD_IRQ);
}
