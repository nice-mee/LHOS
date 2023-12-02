#include "gui.h"
#include "background.h"
#include "../types.h"
#include "../lib.h"
#include "text.h"
#include "../date.h"

int32_t last_x = 0, last_y = 0;
uint32_t mouse_initial = 0;
uint32_t mouse_buffer[22][34];

uint32_t* qemu_memory = (uint32_t *) QEMU_BASE_ADDR;
int box = 1;
int back = 0;
int col = 0x8B00FF;

void draw_background() {
    int i;
    int j;
    for (i = 0; i < Y_RESOLUTION; i++) {
        for (j = 0; j < X_RESOLUTION; j++) {
            uint32_t pixel_color = 
             (((((bg[j + i * X_RESOLUTION] & RED_MASK) >> RED_OFF) << 3) & LOW_8_BITS) << 16)
            |(((((bg[j + i * X_RESOLUTION] & GRE_MASK) >> GRE_OFF) << 2) & LOW_8_BITS) << 8)
            |(((( bg[j + i * X_RESOLUTION] & BLU_MASK) << BLU_OFF) & LOW_8_BITS));
            *(uint32_t *)(qemu_memory + i * X_RESOLUTION + j) = pixel_color;
        }
    }
}

void draw_string(int x, int y, int8_t* str, uint32_t color) {
    int i, j ,k;
    int cur_x, cur_y;
    for (i = 0; i < strlen(str); ++i) {
        cur_x = x + 18 * i, cur_y = y;
        char* print_char = (char*)font_data[(uint8_t)(str[i])];
        for (j = 0; j < FONT_HEIGHT; ++j) {
            for (k = 0; k < FONT_WIDTH; ++k) {
                if (print_char[j] & 1 << (FONT_WIDTH - k)) {
                    uint32_t rgb = 0;
                    rgb |= color;
                    uint32_t* pixel = (uint32_t *)(qemu_memory + (cur_x + 2 * k) + (cur_y + 2 * j) * X_RESOLUTION);
                    pixel[0] = color;
                    pixel[1] = color;
                    pixel[X_RESOLUTION] = color;
                    pixel[X_RESOLUTION + 1] = color;
                }
            }
        }
    }
}

void draw_time() {
    /* show detailed time */
    char time_str[9];
    time_str[0] = '0' + hour / 10;
    time_str[1] = '0' + hour % 10;
    time_str[2] = ':';
    time_str[3] = '0' + min / 10;
    time_str[4] = '0' + min % 10;
    time_str[5] = ':';
    time_str[6] = '0' + sec / 10;
    time_str[7] = '0' + sec % 10;
    time_str[8] = '\0';
    int i, j;
    for (i = 150; i < 182; i++) {
        for (j = 434; j < 590; j++) {
            uint32_t pixel_color = 
             (((((bg[j + i * X_RESOLUTION] & RED_MASK) >> RED_OFF) << 3) & LOW_8_BITS) << 16)
            |(((((bg[j + i * X_RESOLUTION] & GRE_MASK) >> GRE_OFF) << 2) & LOW_8_BITS) << 8)
            |(((( bg[j + i * X_RESOLUTION] & BLU_MASK) << BLU_OFF) & LOW_8_BITS));
            *(uint32_t *)(qemu_memory + i * X_RESOLUTION + j) = pixel_color;
        }
    }
    draw_string(434, 150 , time_str, 0xFFFFFFFF);
    /* show date */
    char time_str1[12];
    time_str1[0] = '1';
    time_str1[1] = ' ';
    time_str1[2] = 'D';
    time_str1[3] = 'E';
    time_str1[4] = 'C';
    time_str1[5] = ' ';
    time_str1[6] = '2';
    time_str1[7] = '0';
    time_str1[8] = '2';
    time_str1[9] = '3';
    time_str1[10] = '\0';
    draw_string(420, 100 , time_str1, 0xFFFFFFFF);
}

void gui_set_up() {
    draw_background();
}