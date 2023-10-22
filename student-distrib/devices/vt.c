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
    char input_buf[INPUT_BUF_SIZE]; // Temporary buffer for storing user input
    int input_buf_ptr;
    volatile int enter_pressed;
    char user_buf[INPUT_BUF_SIZE]; // Buffer for storing user input after enter is pressed
} vt_state_t;

static vt_state_t vt_state[1];
static int cur_vt = 0;

/* vt_init
 *   DESCRIPTION: Initialize virtual terminal.
 *                This function has to be called before printing anything on the screen!!!
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void vt_init(void) {
    vt_state[cur_vt].screen_x = 0;
    vt_state[cur_vt].screen_y = 0;
    vt_state[cur_vt].video_mem = (char*)VIDEO;
    vt_state[cur_vt].kbd.shift = 0;
    vt_state[cur_vt].kbd.caps = 0;
    vt_state[cur_vt].kbd.ctrl = 0;
    vt_state[cur_vt].kbd.alt = 0;
    vt_state[cur_vt].input_buf_ptr = 0;
    vt_state[cur_vt].enter_pressed = 0;
}

/* vt_open
 *   DESCRIPTION: Open virtual terminal.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void vt_open(void) {
    // Do nothing because everything is done in vt_init()
}

/* vt_close
 *   DESCRIPTION: Close virtual terminal.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void vt_close(void) {
    // Do nothing
}

/* vt_read
 *   DESCRIPTION: Read from virtual terminal.
 *   INPUTS: fd -- file descriptor
 *           buf -- buffer to read into
 *           nbytes -- number of bytes to read
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes read
 *   SIDE EFFECTS: none
 */
int32_t vt_read(int32_t fd, void* buf, int32_t nbytes) {
    if (buf == NULL || nbytes < 0)
        return -1;

    // Wait for enter key
    while (!vt_state[cur_vt].enter_pressed);

    // Critical section should be enforced to prevent interrupt from modifying user_buf
    // This is highly unlikely (since human input is pretty slow) but still possible
    cli();
    vt_state[cur_vt].enter_pressed = 0;

    // Copy user buffer to buf
    int i;
    for (i = 0; i < nbytes && i < INPUT_BUF_SIZE && vt_state[cur_vt].user_buf[i] != '\0'; i++) {
        ((char*)buf)[i] = vt_state[cur_vt].user_buf[i];
    }
    sti();

    return i;
}

/* vt_write
 *   DESCRIPTION: Write to virtual terminal.
 *   INPUTS: fd -- file descriptor
 *           buf -- buffer to write from
 *           nbytes -- number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes written
 *   SIDE EFFECTS: none
 */
int32_t vt_write(int32_t fd, const void* buf, int32_t nbytes) {
    if (buf == NULL || nbytes < 0)
        return -1;
    int i;
    for (i = 0; i < nbytes; i++) {
        vt_putc(((char*)buf)[i]);
    }
    return i;
}

/* scroll_page (PRIVATE)
 *   DESCRIPTION: Scroll the screen up by one line.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: video memory is modified
 */
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

/* print_newline (PRIVATE)
 *   DESCRIPTION: Print a newline character on the screen.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: video memory is modified if scrooling happens
 */
static void print_newline(void) {
    vt_state[cur_vt].screen_y++;
    vt_state[cur_vt].screen_x = 0;
    if (vt_state[cur_vt].screen_y >= NUM_ROWS) {
        scroll_page();
        vt_state[cur_vt].screen_y = NUM_ROWS - 1;
    }
}

/* print_backspace (PRIVATE)
 *   DESCRIPTION: Print a backspace character on the screen.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: video memory is modified
 */
static void print_backspace(void) {
    char * video_mem = vt_state[cur_vt].video_mem;
    vt_state[cur_vt].screen_x--;
    if (vt_state[cur_vt].screen_x < 0) {
        vt_state[cur_vt].screen_x = NUM_COLS - 1;
        vt_state[cur_vt].screen_y--;
        if (vt_state[cur_vt].screen_y < 0) {
            vt_state[cur_vt].screen_y = 0;
            vt_state[cur_vt].screen_x = 0;
        }
    }
    *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[cur_vt].screen_y + vt_state[cur_vt].screen_x) << 1)) = ' ';
    *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[cur_vt].screen_y + vt_state[cur_vt].screen_x) << 1) + 1) = ATTRIB;
}

/* redraw_cursor (PRIVATE)
 *   DESCRIPTION: Redraw the cursor on the screen.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: video memory is modified
 */
static void redraw_cursor(void) {
    uint16_t pos = vt_state[cur_vt].screen_y * NUM_COLS + vt_state[cur_vt].screen_x;

	outb(0x0F, 0x3D4);
	outb((uint8_t) (pos & 0xFF), 0x3D5);
	outb(0x0E, 0x3D4);
	outb((uint8_t) ((pos >> 8) & 0xFF), 0x3D5);
}

/* process_default (PRIVATE)
 *   DESCRIPTION: Process default cases in vt_keyboard().
 *                This function is inside interrupt context!!!
 *   INPUTS: keycode -- keycode of the key pressed
 *           release -- whether the key is released
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
static void process_default(keycode_t keycode, int release) {
    // Ignore key releases
    if (release)
        return;
    
    // Handle special key combinations
    if (vt_state[cur_vt].kbd.ctrl && keycode == KEY_L) { // Ctrl + L
        clear();
        vt_state[cur_vt].input_buf_ptr = 0;
        memset(vt_state[cur_vt].input_buf, '\0', INPUT_BUF_SIZE * sizeof(char));
        vt_state[cur_vt].screen_x = 0;
        vt_state[cur_vt].screen_y = 0;
        redraw_cursor();
        return;
    }

    /* Echo printable characters */
    if (keycode == KEY_RESERVED || keycode > KEY_SPACE) // KEY_SPACE is the last printable character in keycode table
        return;
    if (vt_state[cur_vt].input_buf_ptr >= INPUT_BUF_SIZE - 1)
        return;

    char c;
    if (keycode >= KEY_A && keycode <= KEY_Z) {
        // A-Z should be affected by caps lock as well
        c = keycode_to_printable_char[vt_state[cur_vt].kbd.shift ^ vt_state[cur_vt].kbd.caps][keycode];
        vt_putc(c);
    } else {
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
void vt_keyboard(keycode_t keycode, int release) {
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
                vt_state[cur_vt].input_buf[vt_state[cur_vt].input_buf_ptr] = '\n';
                memcpy(vt_state[cur_vt].user_buf, vt_state[cur_vt].input_buf, INPUT_BUF_SIZE * sizeof(char)); // Copy input buffer to user buffer
                vt_state[cur_vt].input_buf_ptr = 0; // Reset input buffer pointer
                memset(vt_state[cur_vt].input_buf, '\0', INPUT_BUF_SIZE * sizeof(char)); // Clear input buffer
                vt_state[cur_vt].enter_pressed = 1; // Signal read() that enter is pressed
            }
            break;
        case KEY_BACKSPACE:
            if (!release && vt_state[cur_vt].input_buf_ptr > 0) {
                vt_putc('\b');
                vt_state[cur_vt].input_buf_ptr--;
                vt_state[cur_vt].input_buf[vt_state[cur_vt].input_buf_ptr] = '\0';
            }
            break;
        case KEY_TAB:
            process_default(KEY_SPACE, release); // Treat tab as space, easier to implement
            break;
        default:
            process_default(keycode, release);
            break;
    }
}

/* vt_putc
 *   DESCRIPTION: Put a character on the screen.
                  Can handle newline and carriage return.
                  Can handle backspace.
                  Rejects null character.
 *   INPUTS: c -- character to put on the screen
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void vt_putc(char c) {
    if (c == '\0')
        return;
    unsigned long flags;
    cli_and_save(flags);
    if (c == '\n' || c == '\r') {
        print_newline();
    } else if (c == '\b') {
        print_backspace();
    } else {
        char * video_mem = vt_state[cur_vt].video_mem;
        *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[cur_vt].screen_y + vt_state[cur_vt].screen_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[cur_vt].screen_y + vt_state[cur_vt].screen_x) << 1) + 1) = ATTRIB;
        vt_state[cur_vt].screen_x++;
        if (vt_state[cur_vt].screen_x >= NUM_COLS)
            print_newline();
    }
    redraw_cursor();
    restore_flags(flags);
}
