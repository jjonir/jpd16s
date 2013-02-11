#ifndef _DCPU16_H
#define _DCPU16_H

#include <stdint.h>

#define MEM_SIZE (0x10000 * sizeof(uint16_t))

struct register_file {
	uint16_t A, B, C;
	uint16_t X, Y, Z;
	uint16_t I, J;
	uint16_t PC, SP, EX, IA;
};
#define REG_SIZE (sizeof(struct register_file))

#define MAX_INTERRUPTS 256
struct interrupts {
	uint16_t queue[MAX_INTERRUPTS];
	uint8_t first;
	uint8_t last;
	uint8_t enabled;
};
#define INT_SIZE (sizeof(struct interrupts))

extern uint16_t *memory;
extern struct register_file *registers;
extern struct interrupts *interrupts;

void sim_init(void);
void sim_start(void);
void sim_stop(void);

#endif
