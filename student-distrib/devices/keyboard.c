#include "keyboard.h"
#include "vt.h"
#include "../lib.h"
#include "../i8259.h"

static const uint8_t ps2_set1_keycode[128] = {
    KEY_RESERVED, KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6,
    KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE,
    KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I,
    KEY_O, KEY_P, KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_ENTER, KEY_LEFTCTRL,
    KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L,
    KEY_SEMICOLON, KEY_APOSTROPHE, KEY_GRAVE, KEY_LEFTSHIFT, KEY_BACKSLASH,
    KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA,
    KEY_DOT, KEY_SLASH, KEY_RIGHTSHIFT, KEY_KPASTERISK, KEY_LEFTALT,
    KEY_SPACE, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK, KEY_SCROLLLOCK,
    KEY_KP7, KEY_ARROW_UP, KEY_KP9, KEY_KPMINUS, KEY_ARROW_LEFT, KEY_KP5, KEY_ARROW_RIGHT,
    KEY_KPPLUS, KEY_KP1, KEY_ARROW_DOWN, KEY_KP3, KEY_KP0, KEY_KPDOT,
    // Remaining keys are not used but will be mapped to KEY_RESERVED (0) anyway
};

static const int extended_key;

/* keyboard_intr_handler
 *   DESCRIPTION: keyboard interrupt handler
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: may print a character to the screen
 */
DEFINE_DEVICE_HANDLER(keyboard) {
    keycode_t keycode;
    int release = 0;
    send_eoi(KEYBOARD_IRQ);
    unsigned char scan_code = inb(KEYBOARD_PORT);
    if (scan_code >> 7) {
        release = 1;
        scan_code &= 0x7F;
    }
    keycode = ps2_set1_keycode[scan_code];
    vt_keyboard(keycode, release);
}

/* keyboard_init
 *   DESCRIPTION: enables keyboard interrupts
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: enables keyboard interrupts
 */
DEFINE_DEVICE_INIT(keyboard) {
    enable_irq(KEYBOARD_IRQ);
}
