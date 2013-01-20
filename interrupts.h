#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include <stdint.h>

extern uint8_t interrupts_enabled;

void queue_interrupt(uint16_t msg);
uint8_t get_queued_interrupts(void);
void trigger_next_queued_interrupt(void);

#endif
