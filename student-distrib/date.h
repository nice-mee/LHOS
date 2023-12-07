#ifndef _DATE_H
#define _DATE_H
#include "devices/rtc.h"

#define CMOS_SELE_PORT 0x70 // select a CMOS register through this port
#define CMOS_READ_PORT 0x71 // read the value of the register through this port

#define SEC_PORT 0x00
#define MIN_PORT 0x02
#define HOUR_PORT 0x04
#define DAY_PORT 0x07
#define MON_PORT 0x08
#define YEAR_PORT 0x09

int32_t sec;
int32_t min;
int32_t hour;
int32_t day;
int32_t month;
int32_t year;

extern void get_date();

#endif
