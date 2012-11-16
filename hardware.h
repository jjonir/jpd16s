#ifndef _HARDWARE_H
#define _HARDWARE_H

#include <stdint.h>

enum regnum {
	REG_A = 0,
	REG_B = 1,
	REG_C = 2,
	REG_X = 3,
	REG_Y = 4,
	REG_Z = 5,
	REG_I = 6,
	REG_J = 7,
	REG_PC = 8,
	REG_SP = 9,
	REG_EX = 10,
	REG_IA = 11
};

struct hardware {
	uint16_t id[2];
	uint16_t version;
	uint16_t mfg[2];

	void (*init)(void);
	int (*interrupt)(void);
	void (*step)(void);
};

extern struct hardware *hardware[0x10000];

uint16_t read_register(uint16_t reg);
void read_memory(uint16_t addr, uint16_t len, uint16_t *buf);
void write_register(uint16_t reg, uint16_t val);
void write_memory(uint16_t addr, uint16_t len, uint16_t *buf);
void raise_interrupt(uint16_t msg);

#endif
