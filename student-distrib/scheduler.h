#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "paging.h"
#include "devices/vt.h"
#include "pcb.h"
#include "x86_desc.h"
#define EIGHT_MB 0x800000
#define EIGHT_KB 0x2000

extern void scheduler();

#endif
