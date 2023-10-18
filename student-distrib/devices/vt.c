/* vt.c - Virtual terminal related functions.
 */

#include "vt.h"

#define VIDEO       0xB8000
#define NUM_COLS    80
#define NUM_ROWS    25
#define ATTRIB      0x7
#define INPUT_BUF_SIZE 128

static int8_t keycode_to_printable_char[2][128] =
{
    { // Unshifted version
        '\0',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k','l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u','v', 'w', 'x', 'y', 'z',
        '`', '-', '=', '[', ']', '\\', ';', '\'', ',', '.', '/', ' ',
        // Remaining keys are not used but will be mapped to '\0' anyway
    },
    { // Shifted version
        '0',
        ')', '!', '@', '#', '$', '%', '^', '&', '*', '(',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K','L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U','V', 'W', 'X', 'Y', 'Z',
        '~', '_', '+', '{', '}', '|', ':', '"', '<', '>', '?', ' ',
        // Remaining keys are not used but will be mapped to '\0' anyway
    }
};

typedef struct {
    int screen_x;
    int screen_y;
    // char screen[80][25];
    char* video_mem;
    keyboard_state_t kbd;
    char* input_buf[INPUT_BUF_SIZE];
    int input_buf_ptr;
} vt_state_t;

static vt_state_t vt_state[1];
static int cur_vt = 0;

/* vt_init
 *   DESCRIPTION: Initialize virtual terminal.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void vt_init(void) {
    cli();
    vt_state[cur_vt].screen_x = 0;
    vt_state[cur_vt].screen_y = 0;
    vt_state[cur_vt].video_mem = (char*)VIDEO;
    vt_state[cur_vt].kbd.shift = 0;
    vt_state[cur_vt].kbd.caps = 0;
    vt_state[cur_vt].kbd.ctrl = 0;
    vt_state[cur_vt].kbd.alt = 0;
    vt_state[cur_vt].input_buf_ptr = 0;
    sti();
}

/* vt_open
 *   DESCRIPTION: Open virtual terminal.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void vt_open(void) {

}

/* vt_close
 *   DESCRIPTION: Close virtual terminal.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void vt_close(void) {
    
}

/* vt_read
 *   DESCRIPTION: Read from virtual terminal.
 *   INPUTS: buf -- buffer to read into
 *           nbytes -- number of bytes to read
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes read
 *   SIDE EFFECTS: none
 */
int32_t vt_read(void* buf, int32_t nbytes) {

    return 0;
}

/* vt_write
 *   DESCRIPTION: Write to virtual terminal.
 *   INPUTS: buf -- buffer to write from
 *           nbytes -- number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes written
 *   SIDE EFFECTS: none
 */
int32_t vt_write(const void* buf, int32_t nbytes) {

    return 0;
}

static void scroll_page(void) {
    char * video_mem = vt_state[cur_vt].video_mem;
    int i;
    for (i = 0; i < NUM_ROWS - 1; i++) {
        memcpy(video_mem + ((NUM_COLS * i) * 2), video_mem + ((NUM_COLS * (i + 1)) * 2), NUM_COLS * 2 * sizeof(char));
    }
    for (i = 0; i < NUM_COLS; i++) {
        *(uint8_t *)(video_mem + ((NUM_COLS * (NUM_ROWS - 1) + i) << 1)) = ' ';
        *(uint8_t *)(video_mem + ((NUM_COLS * (NUM_ROWS - 1) + i) << 1) + 1) = ATTRIB;
    }
}

static void print_newline(void) {
    vt_state[cur_vt].screen_y++;
    vt_state[cur_vt].screen_x = 0;
    if (vt_state[cur_vt].screen_y >= NUM_ROWS) {
        scroll_page();
        vt_state[cur_vt].screen_y = NUM_ROWS - 1;
    }
}

static void print_backspace(void) {
    char * video_mem = vt_state[cur_vt].video_mem;
    vt_state[cur_vt].screen_x--;
    if (vt_state[cur_vt].screen_x < 0) {
        vt_state[cur_vt].screen_x = NUM_COLS - 1;
        vt_state[cur_vt].screen_y--;
        if (vt_state[cur_vt].screen_y < 0) // In theory this should never happen
            vt_state[cur_vt].screen_y = 0;
    }
    *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[cur_vt].screen_y + vt_state[cur_vt].screen_x) << 1)) = ' ';
    *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[cur_vt].screen_y + vt_state[cur_vt].screen_x) << 1) + 1) = ATTRIB;
}

static void redraw_cursor(void) {
    uint16_t pos = vt_state[cur_vt].screen_y * NUM_COLS + vt_state[cur_vt].screen_x;
 
	outb(0x0F, 0x3D4);
	outb((uint8_t) (pos & 0xFF), 0x3D5);
	outb(0x0E, 0x3D4);
	outb((uint8_t) ((pos >> 8) & 0xFF), 0x3D5);
}

static void process_char(uint8_t keycode, int release) {
    // Ignore key releases
    if (release)
        return;
    
    // Handle special key combinations
    if (vt_state[cur_vt].kbd.ctrl && keycode == KEY_L) {
        clear();
        vt_state[cur_vt].screen_x = 0;
        vt_state[cur_vt].screen_y = 0;
        redraw_cursor();
        return;
    }

    /* Echo printable characters */
    if (keycode == KEY_RESERVED || keycode > KEY_SPACE) // KEY_SPACE is the last printable character in keycode table
        return;
    if (vt_state[cur_vt].input_buf_ptr >= INPUT_BUF_SIZE)
        return;

    char c;
    if (keycode >= KEY_A && keycode <= KEY_Z) {
        // A-Z should be affected by caps lock as well
        c = keycode_to_printable_char[vt_state[cur_vt].kbd.shift ^ vt_state[cur_vt].kbd.caps][keycode];
        vt_putc(c);
    }
    else {
        // Other keys should not be affected by caps lock
        c = keycode_to_printable_char[vt_state[cur_vt].kbd.shift][keycode];
        vt_putc(c);
    }
    vt_state[cur_vt].input_buf[vt_state[cur_vt].input_buf_ptr] = c;
    vt_state[cur_vt].input_buf_ptr++;
}

/* vt_keyboard
 *   DESCRIPTION: Keyboard input handler for virtual terminal. 
                  This function is inside interrupt context!!!
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void vt_keyboard(uint8_t keycode, int release) {
    switch (keycode) {
        case KEY_LEFTSHIFT:
        case KEY_RIGHTSHIFT:
            vt_state[cur_vt].kbd.shift = !release;
            break;
        case KEY_LEFTCTRL:
            vt_state[cur_vt].kbd.ctrl = !release;
            break;
        case KEY_CAPSLOCK:
            if (!release)
                vt_state[cur_vt].kbd.caps = !vt_state[cur_vt].kbd.caps;
            break;
        case KEY_ENTER:
            if (!release) {
                vt_putc('\n');
                vt_state[cur_vt].input_buf_ptr = 0;
            }
            break;
        case KEY_BACKSPACE:
            if (!release && vt_state[cur_vt].input_buf_ptr > 0) {
                vt_putc('\b');
                vt_state[cur_vt].input_buf_ptr--;
            }
            break;
        default:
            process_char(keycode, release);
            break;
    }
}

/* vt_putc
 *   DESCRIPTION: Put a character on the screen.
                  Can handle newline and carriage return.
                  Can handle backspace.
 *   INPUTS: c -- character to put on the screen
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void vt_putc(uint8_t c) {
    unsigned long flags;
    cli_and_save(flags);
    if (c == '\n' || c == '\r') {
        print_newline();
        redraw_cursor();
    } else if (c == '\b') {
        print_backspace();
        redraw_cursor();
    } else {
        char * video_mem = vt_state[cur_vt].video_mem;
        *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[cur_vt].screen_y + vt_state[cur_vt].screen_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[cur_vt].screen_y + vt_state[cur_vt].screen_x) << 1) + 1) = ATTRIB;
        vt_state[cur_vt].screen_x++;
        if (vt_state[cur_vt].screen_x >= NUM_COLS)
            print_newline();
        redraw_cursor();
    }
    restore_flags(flags);
}
