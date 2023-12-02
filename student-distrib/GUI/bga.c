#include "bga.h"
#include "../types.h"
#include "../lib.h"


#define bga_write_reg(index, value) do { \
    outw((index), BGA_WRITE_PORT);   \
    outw((value), BGA_DATA_PORT);    \
} while (0)

void program_bga(uint16_t xres, uint16_t yres, uint16_t bpp) {
    bga_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    bga_write_reg(VBE_DISPI_INDEX_XRES, xres);
    bga_write_reg(VBE_DISPI_INDEX_YRES, yres);
    bga_write_reg(VBE_DISPI_INDEX_BPP, bpp);
    bga_write_reg(VBE_DISPI_INDEX_FB_BASE_HI, QEMU_BASE_ADDR >> 16);
    bga_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED);
}


