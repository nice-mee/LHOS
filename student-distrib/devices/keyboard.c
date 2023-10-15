#include "keyboard.h"
#include "../lib.h"
#include "../i8259.h"

void __intr_keyboard_handler() {
    cli();
    send_eoi(KEYBOARD_IRQ);
    unsigned char scan_code = inb(0x60);
    printf("Keyboard interrupt, scan code: 0x%x\n", scan_code);
    sti();
}

void keyboard_init() {
    printf("Keyboard_init\n");
    enable_irq(KEYBOARD_IRQ);
}