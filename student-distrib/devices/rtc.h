#ifndef _RTC_H_
#define _RTC_H_
#include "../lib.h"

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
#define RTC_BASE_FREQ    1024
#define RTC_BASE_RATE    6

/* maximum number of processes */
#define MAX_PROC_NUM    8

/* Initialize the rtc */
void RTC_init(void);
/* deal with rtc interrupts*/
void __intr_RTC_handler(void);
/* for checkpoint2 */
int32_t RTC_open(const uint8_t* proc_id);
int32_t RTC_close(int32_t proc_id);
int32_t RTC_read(int32_t proc_id, void* buf, int32_t nbytes);
int32_t RTC_write(int32_t proc_id, const void* buf, int32_t nbytes);

#endif /* _RTC_H_ */
