/* vt.c - Virtual terminal related functions.
 */

#include "vt.h"
#include "../signal.h"

static int32_t VIDEO = 0xB8000;
#define FOUR_KB     0x1000
#define VID_BUF_SIZE (80 * 25 * 2)
#define NUM_COLS    80
#define NUM_ROWS    25
#define ATTRIB      0x7

static int32_t gui_activated = 0;

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


vt_state_t vt_state[NUM_TERMS];
int cur_vt = 0;
int foreground_vt = 0;

static void redraw_cursor(int term_idx);

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
        vt_state[i].halt_pending = 0;
        vt_state[i].raw = 0;
        vt_state[i].attrib = ATTRIB;
        vt_state[i].cur_cmd_idx = 0;
        vt_state[i].cur_cmd_cnt = 0;
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

static int32_t vt_read_raw(void* buf, int32_t nbytes) {
    while (vt_state[cur_vt].input_buf_ptr == 0);
    cli();
    int i;
    for (i = 0; i < nbytes && i < vt_state[cur_vt].input_buf_ptr; i++) {
        ((char*)buf)[i] = vt_state[cur_vt].input_buf[i];
    }
    memcpy(vt_state[cur_vt].input_buf, &vt_state[cur_vt].input_buf[i], vt_state[cur_vt].input_buf_ptr - i);
    vt_state[cur_vt].input_buf_ptr -= i;
    sti();
    return i;
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

    if (vt_state[cur_vt].raw)
        return vt_read_raw(buf, nbytes);
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
        if (vt_state[cur_vt].raw && ((char*)buf)[i] == '\x1b' && ((char*)buf)[i+1] == '[') {
            char * video_mem = vt_state[cur_vt].video_mem;
            char attrib = vt_state[cur_vt].attrib;
            if (strncmp(((char*)buf) + i, "\x1b[2J", 4) == 0) {
                int x, y;
                for (y = 0; y < NUM_ROWS; y++) {
                    for (x = 0; x < NUM_COLS; x++) {
                        *(uint8_t *)(video_mem + ((NUM_COLS * y + x) << 1)) = ' ';
                        *(uint8_t *)(video_mem + ((NUM_COLS * y + x) << 1) + 1) = attrib;
                    }
                }
                i += 3;
                continue;
            }
            if (strncmp(((char*)buf) + i, "\x1b[H", 3) == 0) {
                vt_state[cur_vt].screen_x = 0;
                vt_state[cur_vt].screen_y = 0;
                redraw_cursor(cur_vt);
                i += 2;
                continue;
            }
            if (strncmp(((char*)buf) + i, "\x1b[K", 3) == 0) {
                int x;
                for (x = vt_state[cur_vt].screen_x; x < NUM_COLS; x++) {
                    *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[cur_vt].screen_y + x) << 1)) = ' ';
                    *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[cur_vt].screen_y + x) << 1) + 1) = attrib;
                }
                i += 2;
                continue;
            }
            if (((char*)buf)[i+7] == 'H' && ((char*)buf)[i+4] == ';') {
                int screen_y = atoi(((char*)buf)[i+2]) * 10 + atoi(((char*)buf)[i+3]);
                int screen_x = atoi(((char*)buf)[i+5]) * 10 + atoi(((char*)buf)[i+6]);
                if (screen_y >= 0 && screen_y < NUM_ROWS && screen_x >= 0 && screen_x < NUM_COLS) {
                    vt_state[cur_vt].screen_y = screen_y;
                    vt_state[cur_vt].screen_x = screen_x;
                }
                redraw_cursor(cur_vt);
                i += 7;
                continue;
            }
            if (((char *)buf)[i+7] == 'M' && ((char *)buf)[i+4] == ';') {
                char foreground, background;
                if (((char *)buf)[i+2] == '3') {
                    foreground = atoi(((char *)buf)[i+3]);
                } else {
                    foreground = atoi(((char *)buf)[i+3]) + 8;
                }
                if (((char *)buf)[i+5] == '4') {
                    background = atoi(((char *)buf)[i+6]);
                } else {
                    background = atoi(((char *)buf)[i+6]) + 8;
                }
                vt_state[cur_vt].attrib = (background << 4) | foreground;
                i += 7;
                continue;
            }
        }
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
    char attrib = vt_state[term_idx].attrib;
    int i;
    for (i = 0; i < NUM_ROWS - 1; i++) {
        memcpy(video_mem + ((NUM_COLS * i) * 2), video_mem + ((NUM_COLS * (i + 1)) * 2), NUM_COLS * 2 * sizeof(char));
    }
    for (i = 0; i < NUM_COLS; i++) {
        *(uint8_t *)(video_mem + ((NUM_COLS * (NUM_ROWS - 1) + i) << 1)) = ' ';
        *(uint8_t *)(video_mem + ((NUM_COLS * (NUM_ROWS - 1) + i) << 1) + 1) = attrib;
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
    char attrib = vt_state[term_idx].attrib;
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
    *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[term_idx].screen_y + vt_state[term_idx].screen_x) << 1) + 1) = attrib;
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
    if (cur_vt == foreground_vt) {
        vidmap_table[0].ADDR = (uint32_t)vt_state[cur_vt].video_mem >> 12; // Update the current vt vidmem 
    }
    vt_state[term_idx].video_mem = (char *)(VIDEO);
    if (cur_vt == term_idx) {
        vidmap_table[0].ADDR = (uint32_t)vt_state[cur_vt].video_mem >> 12; // Update the current vt vidmem 
    }
    foreground_vt = term_idx;
    redraw_cursor(term_idx);
}

uint32_t vt_get_cur_vidmem(void)
{
    return (uint32_t)vt_state[cur_vt].video_mem;
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

    if (vt_state[foreground_vt].kbd.ctrl && keycode == KEY_C) { // Ctrl + C
        send_signal_by_pid(SIGNUM_INTERRUPT, vt_state[foreground_vt].active_pid);
        return;
    }

    if (vt_state[foreground_vt].kbd.ctrl && keycode == KEY_M) { // Ctrl + M
        show_memory_usage();        // show memory usage
        return;
    }

    if (vt_state[foreground_vt].kbd.ctrl && keycode == KEY_G) { // Ctrl + G
        if (gui_activated) return;
        VIDEO = 0xE0000;
        vt_state[foreground_vt].video_mem = (char *)VIDEO;
        memcpy((void *)VIDEO, (void *)0xB8000, VID_BUF_SIZE);
        memcpy((void *)(VIDEO + FOUR_KB), (void *)(0xB8000 + FOUR_KB), VID_BUF_SIZE);
        memcpy((void *)(VIDEO + 2 * FOUR_KB), (void *)(0xB8000 + 2 * FOUR_KB), VID_BUF_SIZE);
        memcpy((void *)(VIDEO + 3 * FOUR_KB), (void *)(0xB8000 + 3 * FOUR_KB), VID_BUF_SIZE);
        program_bga(X_RESOLUTION, Y_RESOLUTION, BITS_PER_PIXEL);
        gui_set_up();
        gui_activated = 1;
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

static inline void vt_keyboard_raw(keycode_t keycode, int release) {
    if (vt_state[foreground_vt].input_buf_ptr >= INPUT_BUF_SIZE)
        return;
    if (release) {
        keycode |= 0x80;
    }
    vt_state[foreground_vt].input_buf[vt_state[foreground_vt].input_buf_ptr] = keycode;
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
    if (vt_state[foreground_vt].raw)
        return vt_keyboard_raw(keycode, release);
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
                if(vt_state[foreground_vt].cur_cmd_cnt < NUM_HIST){
                    memset(vt_state[foreground_vt].buf_history[vt_state[foreground_vt].cur_cmd_cnt], '\0', sizeof(vt_state[foreground_vt].buf_history[vt_state[foreground_vt].cur_cmd_cnt]));
                    memcpy(vt_state[foreground_vt].buf_history[vt_state[foreground_vt].cur_cmd_cnt], vt_state[foreground_vt].input_buf, vt_state[foreground_vt].input_buf_ptr * sizeof(char));
                    vt_state[foreground_vt].cur_cmd_cnt++;
                }
                else{
                    int i;
                    for(i = 0; i < NUM_HIST - 1; i++){
                        memset(vt_state[foreground_vt].buf_history[i], '\0', sizeof(vt_state[foreground_vt].buf_history[i]));
                        strcpy(vt_state[foreground_vt].buf_history[i], vt_state[foreground_vt].buf_history[i + 1]);
                    }
                    memset(vt_state[foreground_vt].buf_history[NUM_HIST - 1], '\0', sizeof(vt_state[foreground_vt].buf_history[NUM_HIST - 1]));    
                    memcpy(vt_state[foreground_vt].buf_history[NUM_HIST - 1], vt_state[foreground_vt].input_buf, vt_state[foreground_vt].input_buf_ptr * sizeof(char));
                }
                vt_state[foreground_vt].cur_cmd_idx = vt_state[foreground_vt].cur_cmd_cnt;
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
            if (!release){
                command_completion();
            }
            break;
        case KEY_ARROW_UP:
            if (!release){
                command_history(1);
            }
            break;
        case KEY_ARROW_DOWN:
            if (!release){
                command_history(2);
            }
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
        char attrib = vt_state[term_idx].attrib;
        *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[term_idx].screen_y + vt_state[term_idx].screen_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS * vt_state[term_idx].screen_y + vt_state[term_idx].screen_x) << 1) + 1) = attrib;
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
    if (vt_state[cur_vt].halt_pending) {
        vt_state[cur_vt].halt_pending = 0;
        __syscall_halt(0);
    }

    /* save esp and pid of current process on the current terminal */
    vt_state[cur_vt].esp = cur_esp;
    vt_state[cur_vt].ebp = cur_ebp;

    cur_vt = (cur_vt +1) % NUM_TERMS; // get the idx of next terminal
    uint32_t nxt_pid = vt_state[cur_vt].active_pid;

    if(nxt_pid == -1) { // there's no process running on the nxt terminal
        __syscall_execute((uint8_t*)"shell"); // execute a shell then
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
    pcb_t* cur_pcb = get_pcb_by_pid(pid);
    cur_pcb->vt = cur_vt; 
}

int32_t vt_check_active_pid(int vt_id) {
    if(vt_id < 0 || vt_id > 2) return -1;
    return vt_state[vt_id].active_pid;
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


char all_cmds[NUM_CMDS][32] = {"cat","grep","hello","ls","pingpong","counter","shell","sigtest","testprint","syserr","fish"};

void command_completion(){
    if(vt_state[foreground_vt].input_buf_ptr == 0 || vt_state[foreground_vt].input_buf_ptr > 32){
        return;
    }
    int i, have_space = 0;
    for(i = 0; i < vt_state[foreground_vt].input_buf_ptr; i++){
        if(vt_state[foreground_vt].input_buf[i] == ' '){
            have_space = 1;
            break;
        }
    }
    if(have_space)
        return;
    for(i = 0; i < NUM_CMDS; i++){
        if(vt_state[foreground_vt].input_buf_ptr >= strlen(all_cmds[i]))
            continue;
        if(strncmp(all_cmds[i], vt_state[foreground_vt].input_buf, vt_state[foreground_vt].input_buf_ptr) == 0){
            while(vt_state[foreground_vt].input_buf_ptr > 0){
                vt_state[foreground_vt].input_buf_ptr--;
                vt_state[foreground_vt].input_buf[vt_state[foreground_vt].input_buf_ptr] = '\0';
                vt_putc('\b', 1);
            }
            vt_state[foreground_vt].input_buf_ptr = strlen(all_cmds[i]);
            strcpy(vt_state[foreground_vt].input_buf, all_cmds[i]);
            int j;
            for(j = 0; j < strlen(all_cmds[i]); j++)
                vt_putc(all_cmds[i][j], 1);
            break;
        }
    }
    return;
}

int32_t vt_ioctl(int32_t flag) {
    if (flag == 1) {
        vt_state[cur_vt].raw = 1;
    } else {
        vt_state[cur_vt].raw = 0;
    }
    return 0;
}

/* vt_write_foreground
 *   DESCRIPTION: Write to virtual terminal for the foreground vt.
 *                This syscall does not recognize '\0' as the end of string.
 *                It will simply write nbytes bytes to the screen.
 *   INPUTS: fd -- should be 1, which is stdout
 *           buf -- buffer to write from
 *           nbytes -- number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes written
 *   SIDE EFFECTS: none
 */
int32_t vt_write_foreground(int32_t fd, const void* buf, int32_t nbytes) {
    if (buf == NULL || nbytes < 0 || fd != 1)
        return -1;
    int i;
    for (i = 0; i < nbytes; i++) {
        vt_putc(((char*)buf)[i], 1);
    }
    return i;
}

/* show_memory_usage - display the memory usage for all the pcb
 * Inputs: None
 * Outputs: the memory usage
 * Return: 0 if show successfully, -1 otherwise
 */
int32_t show_memory_usage(void){
    int32_t i, j, counter, usage;
    uint8_t buf[4];
    vt_write_foreground(1, "\n+--------------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+", 81);
    vt_write_foreground(1, "| Memory Usage |", 16);
    for(i = 0; i < 16; i++){
        counter = 0;
        for(j = 0; j < 64; j++){
            if(dynamic_tables[i * 64 + j].P == 1){
                counter++;
            }
        }
        usage = (int32_t)(((float) counter / 64) * 100);
        if( usage < 100){
            buf[0] = '0' + (usage / 10) % 10;
            buf[1] = '0' + usage % 10;
            buf[2] = '%';
            buf[3] = '|';
        } else {
            buf[0] = '1';
            buf[1] = '0';
            buf[2] = '0';
            buf[3] = '|';
        }
        vt_write_foreground(1, buf, 4);
    }
    vt_write_foreground(1, "+--------------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+", 80);
    return 0;
}

void command_history(int type){
    if(type == 1){
        if(vt_state[foreground_vt].cur_cmd_cnt == 0)
            return;
        if(vt_state[foreground_vt].cur_cmd_idx == 0)
            return;
        while(vt_state[foreground_vt].input_buf_ptr > 0){
            vt_state[foreground_vt].input_buf_ptr--;
            vt_state[foreground_vt].input_buf[vt_state[foreground_vt].input_buf_ptr] = '\0';
            vt_putc('\b', 1);
        }
        vt_state[foreground_vt].cur_cmd_idx--;
        vt_state[foreground_vt].input_buf_ptr = strlen(vt_state[foreground_vt].buf_history[vt_state[foreground_vt].cur_cmd_idx]);
        memset(vt_state[foreground_vt].input_buf, '\0', sizeof(vt_state[foreground_vt].input_buf));    
        strcpy(vt_state[foreground_vt].input_buf, vt_state[foreground_vt].buf_history[vt_state[foreground_vt].cur_cmd_idx]);
        int j;
        for(j = 0; j < strlen(vt_state[foreground_vt].buf_history[vt_state[foreground_vt].cur_cmd_idx]); j++)
            vt_putc(vt_state[foreground_vt].buf_history[vt_state[foreground_vt].cur_cmd_idx][j], 1);
    }
    else if(type == 2){
        if(vt_state[foreground_vt].cur_cmd_cnt == 0)
            return;
        if(vt_state[foreground_vt].cur_cmd_idx >= vt_state[foreground_vt].cur_cmd_cnt - 1)
            return;
        while(vt_state[foreground_vt].input_buf_ptr > 0){
            vt_state[foreground_vt].input_buf_ptr--;
            vt_state[foreground_vt].input_buf[vt_state[foreground_vt].input_buf_ptr] = '\0';
            vt_putc('\b', 1);
        }
        vt_state[foreground_vt].cur_cmd_idx++;
        vt_state[foreground_vt].input_buf_ptr = strlen(vt_state[foreground_vt].buf_history[vt_state[foreground_vt].cur_cmd_idx]);
        memset(vt_state[foreground_vt].input_buf, '\0', sizeof(vt_state[foreground_vt].input_buf));    
        strcpy(vt_state[foreground_vt].input_buf, vt_state[foreground_vt].buf_history[vt_state[foreground_vt].cur_cmd_idx]);
        int j;
        for(j = 0; j < strlen(vt_state[foreground_vt].buf_history[vt_state[foreground_vt].cur_cmd_idx]); j++)
            vt_putc(vt_state[foreground_vt].buf_history[vt_state[foreground_vt].cur_cmd_idx][j], 1);
    }
}
