#ifndef _VT_H
#define _VT_H
#include "keyboard.h"
#include "../lib.h"
#include "../filesys.h"
#include "../pcb.h"
#include "../syscall_task.h"
#include "../paging.h"
#include "../dynamic_alloc.h"
#include "../GUI/gui.h"

#define INPUT_BUF_SIZE 128
#define NUM_TERMS 3
#define NUM_CMDS 11
#define NUM_HIST 10

extern void vt_init();
extern int32_t vt_open(const uint8_t* id);
extern int32_t vt_close(int32_t id);
extern int32_t vt_read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t vt_write(int32_t fd, const void* buf, int32_t nbytes);
extern void vt_keyboard(keycode_t keycode, int release);
void vt_putc(char c, int kdb);
extern int32_t bad_read_call(int32_t fd, void* buf, int32_t nbytes);
extern int32_t bad_write_call(int32_t fd, const void* buf, int32_t nbytes);
extern int32_t vt_set_active_term(uint32_t cur_esp, uint32_t cur_ebp);
extern void vt_set_active_pid(int pid);
void vt_get_ebp_esp(uint32_t *esp, uint32_t *ebp);
uint32_t vt_get_cur_vidmem(void);
void command_completion();
int32_t vt_ioctl(int32_t flag);
int32_t vt_check_active_pid(int vt_id);

int32_t show_memory_usage(void);
extern int32_t vt_write_foreground(int32_t fd, const void* buf, int32_t nbytes);
void command_history();
extern operation_table_t stdin_operation_table;
extern operation_table_t stdout_operation_table;


typedef struct {
    int screen_x;
    int screen_y;
    char* video_mem;
    keyboard_state_t kbd;
    char input_buf[INPUT_BUF_SIZE]; // Temporary buffer for storing user input
    volatile int input_buf_ptr;
    volatile int enter_pressed;
    char user_buf[INPUT_BUF_SIZE]; // Buffer for storing user input after enter is pressed
    char buf_history[NUM_HIST][INPUT_BUF_SIZE];
    int cur_cmd_idx;
    int cur_cmd_cnt;
    int nbytes_read;
    uint32_t active_pid; // default as -1
    uint32_t esp;
    uint32_t ebp;
    uint32_t halt_pending;
    int32_t raw;
    int8_t attrib;
} vt_state_t;

extern vt_state_t vt_state[NUM_TERMS];
extern int cur_vt;
extern int foreground_vt;

#endif /* _VT_H */ 
