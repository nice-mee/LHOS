#include "date.h"
#include "GUI/gui.h"

int32_t get_update_in_progress_flag() { // learnt from https://wiki.osdev.org/CMOS#Accessing_CMOS_Registers
      outb(0x0A, CMOS_SELE_PORT);
      return (inb(CMOS_READ_PORT) & 0x80);
}

int32_t read_from_RTC(int port) {
    outb(port, CMOS_SELE_PORT);
    return inb(CMOS_READ_PORT);
}

void get_date() {
    while(get_update_in_progress_flag()); // ensure no update is in progress
    sec = read_from_RTC(SEC_PORT);
    min = read_from_RTC(MIN_PORT);
    hour = read_from_RTC(HOUR_PORT);
    day = read_from_RTC(DAY_PORT);
    month = read_from_RTC(MON_PORT);
    year = read_from_RTC(YEAR_PORT);

    int32_t registerB = read_from_RTC(0x0B);
 
    // Convert BCD to binary values if necessary
 
    if (!(registerB & 0x04)) {
        sec = (sec & 0x0F) + ((sec >> 4) * 10);
        min = (min & 0x0F) + ((min >> 4) * 10);
        hour = (hour & 0x0f) + ((hour >> 4) * 10) - 6;
        if(hour < 0) {
            hour += 24;
        }
        day = (day & 0x0F) + ((day >> 4) * 10);
        month = (month & 0x0F) + ((month >> 4) * 10);
        year = (year & 0x0F) + ((year >> 4) * 10);
    }

    draw_time();
    // Convert 12 hour clock to 24 hour clock if necessary
    /*
    if (!(registerB & 0x02) && (hour & 0x80)) {
        hour = ((hour & 0x7F) + 12) % 24;
    }*/
}
