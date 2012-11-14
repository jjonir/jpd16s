#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

#include <stdint.h>

extern uint8_t interrupts_enabled;

void queue_interrupt(uint16_t msg);
void trigger_next_queued_interrupt(void);
void trigger_interrupt(uint16_t msg);

#endif
