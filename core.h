#ifndef _CORE_H
#define _CORE_H

#include <stdint.h>

extern uint16_t clock_time;

uint16_t sim_step(void);
void run_dcpu16(void);

#endif
