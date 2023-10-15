#ifndef _RTC_H_
#define _RTC_H_

#define RTC_IRQ         8       // the RTC will generate IRQ 8

/* used to specify an index or "register number", and to disable NMI */
#define RTC_PORT        0x70

/* used to read or write from/to that byte of CMOS configuration space */
#define RTC_CMOS_PORT   0x71

/* RTC Status Register A, B, and C */
#define RTC_A           0x8A
#define RTC_B           0x8B
#define RTC_C           0x8C

/* base frequency for rtc*/
#define RTC_BASE_FRE    6


/* Initialize the rtc */
void RTC_init(void);
/* deal with rtc interrupts*/
void __intr_RTC_handler(void);