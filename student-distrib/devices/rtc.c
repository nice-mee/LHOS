#include "rtc.h"
#include "../lib.h"
#include "../i8259.h"

uint32_t rtc_time_counter;

void RTC_init(void) {
    printf("RTC_init\n");
    char prev;
    outb(RTC_B, RTC_PORT);     // select register B, and disable NMI
    prev = inb(RTC_CMOS_PORT); // read the current value of register B
    outb(RTC_B, RTC_PORT);     // set the index again (inb will reset the idx)
    /* write the previous value ORed with 0x40. This turns on bit 6 of register B */
    outb(prev | 0x40, RTC_CMOS_PORT);

    outb(RTC_A, RTC_PORT);     // set index to register A, disable NMI
    prev = inb(RTC_CMOS_PORT); // get the previous value of register B
    outb(RTC_A, RTC_PORT);     // set the index again
    outb((prev & 0xF0) | RTC_BASE_FRE, RTC_CMOS_PORT);  // set the frequency to 1024

    rtc_time_counter = 0;
    enable_irq(RTC_IRQ);

}

void __intr_RTC_handler(void) {
    cli();
    rtc_time_counter ++;

    /* for cp1 */
    test_interrupts();

    outb(RTC_C &0x0F, RTC_PORT); // select register C
    inb(RTC_CMOS_PORT);		    // just throw away contents

    send_eoi(RTC_IRQ);
    sti();
}
