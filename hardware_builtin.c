#include "core.h"
#include "interrupts.h"

static uint16_t *register_array = (uint16_t *)&registers;

uint16_t read_register(uint16_t reg)
{
	return register_array[reg];
}

void read_memory(uint16_t addr, uint16_t len, uint16_t *buf)
{
	uint16_t i;

	for (i = 0; i < len; i++)
		buf[i] = memory[addr + i];
}

void write_register(uint16_t reg, uint16_t val)
{
	register_array[reg] = val;
}

void write_memory(uint16_t addr, uint16_t len, uint16_t *buf)
{
	uint16_t i;

	for (i = 0; i < len; i++)
		memory[addr + i] = buf[i];
}

void raise_interrupt(uint16_t msg)
{
	queue_interrupt(msg);
}
