#ifndef _VBE_H
#define _VBE_H

#include "../types.h"

// Data fetched from https://wiki.osdev.org/Bochs_VBE_Extensions

#define BGA_WRITE_PORT                  0x01CE
#define BGA_DATA_PORT                   0x01CF
#define VBE_DISPI_INDEX_ID              0x0
#define VBE_DISPI_INDEX_XRES            0x1
#define VBE_DISPI_INDEX_YRES            0x2
#define VBE_DISPI_INDEX_BPP             0x3
#define VBE_DISPI_INDEX_ENABLE          0x4

#define VBE_DISPI_DISABLED              0x00
#define VBE_DISPI_ENABLED               0x01

#define QEMU_BASE_ADDR                  0xFD000000
#define VBE_DISPI_INDEX_FB_BASE_HI      0x0B

#define X_RESOLUTION                    1024
#define Y_RESOLUTION                    768
#define BITS_PER_PIXEL                  32


extern void program_bga(uint16_t xres, uint16_t yres, uint16_t bpp);

#endif

