#ifndef _PIT_H
#define _PIT_H

/* oscillator's freq: 1.193182 MHz, actually 100Hz */
#define PIT_FREQ 11931  

/* Mode/Command register (write only, read ignored) */
#define MODE_REG 0x43

/*
Bits 
6 - 7: 0 0 = Channel 0
4 - 5: 1 1 = Access mode: lobyte/hibyte
1 - 3: 0 1 1 = Mode 3 (square wave generator)
0:     0 = 16-bit binary*/
#define MODE_CONTAIN 0x36

/* Channel 0 data port (read/write) */
#define CHAN_0_DATA_PORT 0X40

#define PIT_IRQ 0

void pit_init(void);
void __intr_PIT_handler(void);

#endif
