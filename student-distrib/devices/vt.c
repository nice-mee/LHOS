/* vt.c - Virtual terminal related functions.
 */

#include "vt.h"

#define VIDEO       0xB8000
#define FOUR_KB     0x1000
#define VID_BUF_SIZE (80 * 25 * 2)
#define NUM_COLS    80
#define NUM_ROWS    25
#define ATTRIB      0x7

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

operation_table_t stdin_operation_table = {
    .open_operation = vt_open,
    .close_operation = vt_close,
    .read_operation = vt_read,
    .write_operation = bad_write_call
};

operation_table_t stdout_operation_table = {
    .open_operation = vt_open,
    .close_operation = vt_close,
    .read_operation = bad_read_call,
    .write_operation = vt_write
};

typedef struct {
    int screen_x;
    int screen_y;
    char* video_mem;
    keyboard_state_t kbd;
    char input_buf[INPUT_BUF_SIZE]; // Temporary buffer for storing user input
    int input_buf_ptr;
    volatile int enter_pressed;
    char user_buf[INPUT_BUF_SIZE]; // Buffer for storing user input after enter is pressed
    int nbytes_read;
    uint32_t active_pid; // default as -1
    uint32_t esp;
    uint32_t ebp;
} vt_state_t;

static vt_state_t vt_state[NUM_TERMS];
static int cur_vt = 0;
static int foreground_vt = 0;

/* vt_init
 *   DESCRIPTION: Initialize virtual terminal.
 *                This function has to be called before printing anything on the screen!!!
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void vt_init(void) {
    int i;
    for (i = 0; i < NUM_TERMS; i++) {
        vt_state[i].screen_x = 0;
        vt_state[i].screen_y = 0;
        vt_state[i].video_mem = (char*)(VIDEO + (i + 1) * FOUR_KB);
        vt_state[i].kbd.shift = 0;
        vt_state[i].kbd.caps = 0;
        vt_state[i].kbd.ctrl = 0;
        vt_state[i].kbd.alt = 0;
        vt_state[i].input_buf_ptr = 0;
        vt_state[i].enter_pressed = 0;
        vt_state[i].active_pid = -1;
    }
    vt_state[0].video_mem = (char*)VIDEO;
}

/* vt_open
 *   DESCRIPTION: Open virtual terminal.
 *   INPUTS: id - meaningless 
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if successful
 *   SIDE EFFECTS: none
 */
int32_t vt_open(const uint8_t* id) {                        // return 0 as required by checkpoint 2 demo
    // Do nothing because everything is done in vt_init()
    return 0;
}

/* vt_close
 *   DESCRIPTION: Close virtual terminal.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t vt_close(int32_t id) {                              // return 0 as required by checkpoint 2 demo
    // Do nothing
    return 0;
}

/* vt_read
 *   DESCRIPTION: Read from virtual terminal.
 *   INPUTS: fd -- should be 0, which is stdin
 *           buf -- buffer to read into
 *           nbytes -- number of bytes to read
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes read
 *   SIDE EFFECTS: none
 */
int32_t vt_read(int32_t fd, void* buf, int32_t nbytes) {
    if (buf == NULL || nbytes < 0 || fd != 0)
        return -1;

    cli();
    vt_state[cur_vt].input_buf_ptr = 0;
    sti();
    // Wait for enter key
    while (!vt_state[cur_vt].enter_pressed);

    // Critical section should be enforced to prevent interrupt from modifying user_buf
    // This is highly unlikely (since human input is pretty slow) but still possible
    cli();
    vt_state[cur_vt].enter_pressed = 0;

    // Copy user buffer to buf
    int i;
    for (i = 0; i < nbytes && i < vt_state[cur_vt].nbytes_read; i++) {
        ((char*)buf)[i] = vt_state[cur_vt].user_buf[i];
    }
    sti();

    return i;
}

/* vt_write
 *   DESCRIPTION: Write to virtual terminal.
 *                This syscall does not recognize '\0' as the end of string.
 *                It will simply write nbytes bytes to the screen.
 *   INPUTS: fd -- should be 1, which is stdout
 *           buf -- buffer to write from
 *           nbytes -- number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes written
 *   SIDE EFFECTS: none
 */
int32_t vt_write(int32_t fd, const void* buf, int32_t nbytes) {
    if (buf == NULL || nbytes < 0 || fd != 1)
        return -1;
    int i;
    for (i = 0; i < nbytes; i++) {
        vt_putc(((char*)buf)[i], 0);
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
static void scroll_page(int term_idx) {
    char * video_mem = vt_state[term_idx].video_mem;
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
 *   INPUTS: term_idx -- the terminal to write
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: video memory is modified if scrooling happens
 */
static void print_newline(int term_idx) {
    vt_state[term_idx].screen_y++;
    vt_state[term_idx].screen_x = 0;
    if (vt_state[term_idx].screen_y >= NUM_ROWS) {
        scroll_page(term_idx);
        vt_state[term_idx].screen_y = NUM_ROWS - 1;
    }
}

/* print_backspace (PRIVATE)
 *   DESCRIPTION: Print a backspace character on the screen.
 *   INPUTS: term_idx -- the terminal to write
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: video memory is modified
 */
static void print_backspace(int term_idx) {
    char * video_mem = vt_state[term_idx].video_mem;
    vt_state[term_idx].screen_x--;
    if (vt_state[term_idx].screen_x < 0) {
        vt_state[term_idx].screen_x = NUM_COLS - 1;
        vt_state[term_idx].screen_y--;
        if (vt_state[term_idx].screen_y < 0) { // In theory this should never happen
            vt_state[term_idx].screen_y = 0;
            vt_state[term_idx].screen_x = 0;
        }
    }
    *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[term_idx].screen_y + vt_state[term_idx].screen_x) << 1)) = ' ';
    *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[term_idx].screen_y + vt_state[term_idx].screen_x) << 1) + 1) = ATTRIB;
}

/* redraw_cursor (PRIVATE)
 *   DESCRIPTION: Redraw the cursor on the screen.
 *   INPUTS: term_idx -- the terminal to write
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: video memory is modified
 */
static void redraw_cursor(int term_idx) {
    if (term_idx != foreground_vt) {
        return;
    }
    uint16_t pos = vt_state[term_idx].screen_y * NUM_COLS + vt_state[term_idx].screen_x;

	outb(0x0F, 0x3D4);
	outb((uint8_t) (pos & 0xFF), 0x3D5);
	outb(0x0E, 0x3D4);
	outb((uint8_t) ((pos >> 8) & 0xFF), 0x3D5);
}


void vt_switch_term(int32_t term_idx)
{
    memcpy((char *)(VIDEO + (foreground_vt + 1) * FOUR_KB), (char *)(VIDEO), VID_BUF_SIZE);
    clear();
    memcpy((char *)(VIDEO), (char *)(VIDEO + (term_idx + 1) * FOUR_KB), VID_BUF_SIZE);
    vt_state[foreground_vt].video_mem = (char *)(VIDEO + (foreground_vt + 1) * FOUR_KB);
    vt_state[term_idx].video_mem = (char *)(VIDEO);
    foreground_vt = term_idx;
    redraw_cursor(term_idx);
    if (cur_vt == term_idx || cur_vt == foreground_vt) {
        vidmap_table[0].ADDR = (uint32_t)vt_state[cur_vt].video_mem >> 12; // Update the current vt vidmem 
    }
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
    if (vt_state[foreground_vt].kbd.ctrl && keycode == KEY_L) { // Ctrl + L
        clear();
        vt_state[foreground_vt].input_buf_ptr = 0; // Reset input buffer pointer
        vt_state[foreground_vt].screen_x = 0;
        vt_state[foreground_vt].screen_y = 0;
        redraw_cursor(foreground_vt);
        return;
    }

    /* Echo printable characters */
    if (keycode == KEY_RESERVED || keycode > KEY_SPACE) // KEY_SPACE is the last printable character in keycode table
        return;
    if (vt_state[foreground_vt].input_buf_ptr >= INPUT_BUF_SIZE - 1) // -1 because we need to reserve space for '\n'
        return;

    char c;
    if (keycode >= KEY_A && keycode <= KEY_Z) {
        // A-Z should be affected by caps lock as well
        c = keycode_to_printable_char[vt_state[foreground_vt].kbd.shift ^ vt_state[foreground_vt].kbd.caps][keycode];
        vt_putc(c, 1);
    } else {
        // Other keys should not be affected by caps lock
        c = keycode_to_printable_char[vt_state[foreground_vt].kbd.shift][keycode];
        vt_putc(c, 1);
    }
    vt_state[foreground_vt].input_buf[vt_state[foreground_vt].input_buf_ptr] = c;
    vt_state[foreground_vt].input_buf_ptr++;
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
            vt_state[foreground_vt].kbd.shift = !release;
            break;
        case KEY_LEFTCTRL:
            vt_state[foreground_vt].kbd.ctrl = !release;
            break;
        case KEY_CAPSLOCK:
            if (!release)
                vt_state[foreground_vt].kbd.caps = !vt_state[foreground_vt].kbd.caps;
            break;
        case KEY_ENTER:
            if (!release) {
                vt_putc('\n', 1);
                vt_state[foreground_vt].input_buf[vt_state[foreground_vt].input_buf_ptr] = '\n';
                vt_state[foreground_vt].input_buf_ptr++;
                vt_state[foreground_vt].nbytes_read = vt_state[foreground_vt].input_buf_ptr; // Store number of bytes read
                memcpy(vt_state[foreground_vt].user_buf, vt_state[foreground_vt].input_buf, vt_state[foreground_vt].nbytes_read * sizeof(char)); // Copy input buffer to user buffer
                vt_state[foreground_vt].input_buf_ptr = 0; // Reset input buffer pointer
                vt_state[foreground_vt].enter_pressed = 1; // Signal read() that enter is pressed
            }
            break;
        case KEY_BACKSPACE:
            if (!release && vt_state[foreground_vt].input_buf_ptr > 0) {
                vt_putc('\b', 1);
                vt_state[foreground_vt].input_buf_ptr--;
            }
            break;
        case KEY_LEFTALT:
            vt_state[foreground_vt].kbd.alt = !release;
            break;
        case KEY_F1:
        case KEY_F2:
        case KEY_F3:
            if (!release && vt_state[foreground_vt].kbd.alt) {
                vt_switch_term(keycode - KEY_F1);
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
             kbd -- indicate whether this request is from keyboard interrupt
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void vt_putc(char c, int kbd) {
    if (c == '\0')
        return;
    unsigned long flags;
    cli_and_save(flags);
    int term_idx = (kbd) ? (foreground_vt) : (cur_vt);
    if (c == '\n' || c == '\r') {
        print_newline(term_idx);
    } else if (c == '\b') {
        print_backspace(term_idx);
    } else {
        char * video_mem = vt_state[term_idx].video_mem;
        *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[term_idx].screen_y + vt_state[term_idx].screen_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[term_idx].screen_y + vt_state[term_idx].screen_x) << 1) + 1) = ATTRIB;
        vt_state[term_idx].screen_x++;
        if (vt_state[term_idx].screen_x >= NUM_COLS)
            print_newline(term_idx);
    }
    redraw_cursor(term_idx);
    restore_flags(flags);
}

/* vt_set_active_term
 *   DESCRIPTION:
 *   INPUTS: the idx of the terminal to switch to
 *   OUTPUTS: none
 *   RETURN VALUE: the next pid
 *   SIDE EFFECTS: switch to another virtual terminal 
 */
int32_t vt_set_active_term(uint32_t cur_esp, uint32_t cur_ebp)
{
    /* save esp and pid of current process on the current terminal */
    vt_state[cur_vt].esp = cur_esp;
    vt_state[cur_vt].ebp = cur_ebp;

    cur_vt = (cur_vt +1) % NUM_TERMS; // get the idx of next terminal
    uint32_t nxt_pid = vt_state[cur_vt].active_pid;

    if(nxt_pid == -1) { // there's no process running on the nxt terminal
        __syscall_execute("shell"); // execute a shell then
    }

    /* remap video memory */
    vidmap_table[0].ADDR = (uint32_t)vt_state[cur_vt].video_mem >> 12;

    // flushing TLB by reloading CR3 register
    asm volatile (
        "movl %%cr3, %%eax;"  // Move the value of CR3 into EBX
        "movl %%eax, %%cr3;"  // Move the value from EBX back to CR3
        : : : "eax", "memory"
    );
    return nxt_pid;
}

/* set the active_pid of a vt*/
void vt_set_active_pid(int pid) {
    vt_state[cur_vt].active_pid = pid; 
}

void vt_get_ebp_esp(uint32_t *esp, uint32_t *ebp) {
    *esp = vt_state[cur_vt].esp;
    *ebp = vt_state[cur_vt].ebp;
}

/* bad_read_call
 *   DESCRIPTION: return -1 as this function should not be called
 *   INPUTS: all meaningless
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t bad_read_call(int32_t fd, void* buf, int32_t nbytes) {
    return -1;
}

/* bad_write_call
 *   DESCRIPTION: return -1 as this function should not be called
 *   INPUTS: all meaningless
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t bad_write_call(int32_t fd, const void* buf, int32_t nbytes) {
    return -1;
}
