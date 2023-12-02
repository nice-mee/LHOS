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

extern void gui_set_up();
extern void draw_time();
#endif