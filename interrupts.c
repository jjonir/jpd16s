#include "core.h"
#include "interrupts.h"

#define MAX_INTERRUPTS 256
static uint16_t interrupts[MAX_INTERRUPTS];
static uint8_t next_interrupt = 0;
static uint8_t last_unused_interrupt = 0;
uint8_t interrupts_enabled = 1;

void trigger_interrupt(uint16_t msg);

void queue_interrupt(uint16_t msg)
{
	if ((last_unused_interrupt + 1) == next_interrupt)
		CATCH_FIRE(); // TODO
	else
		interrupts[last_unused_interrupt++] = msg;
}

uint8_t get_queued_interrupts(void)
{
	return (last_unused_interrupt + MAX_INTERRUPTS - next_interrupt)
			% MAX_INTERRUPTS;
}

void trigger_next_queued_interrupt(void)
{
	if (last_unused_interrupt != next_interrupt)
		trigger_interrupt(interrupts[next_interrupt++]);
}

void trigger_interrupt(uint16_t msg)
{
	if (registers->IA != 0) {
		memory[--registers->SP] = registers->PC;
		memory[--registers->SP] = registers->A;
		registers->PC = registers->IA;
		registers->A = msg;
		interrupts_enabled = 0;
	}
}
