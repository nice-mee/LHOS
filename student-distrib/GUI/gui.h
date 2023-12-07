#ifndef _GUI_H
#define _GUI_H
#include "bga.h"

#define RED_MASK    0xF800
#define RED_OFF     11
#define GRE_MASK    0x07E0
#define GRE_OFF     5
#define BLU_MASK    0x001F
#define BLU_OFF     3

#define LOW_8_BITS  0xFF

#define VT_ROW      25
#define VT_COL      80
#define VT_WIDTH    660
#define VT_HEIGHT   420

#define VT_START_X  182
#define VT_START_Y  174

#define DAY_START_X 420
#define DAY_START_Y 80
#define SEC_START_X 434
#define SEC_START_Y 130

#define GUI_VID_MEM_ADDR 0xE0000

extern void gui_set_up();
extern void draw_time();
extern void fill_terminal(void);

#endif
