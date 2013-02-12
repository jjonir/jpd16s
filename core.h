#ifndef _CORE_H
#define _CORE_H

#include <stdint.h>


struct register_file {
	uint16_t A, B, C;
	uint16_t X, Y, Z;
	uint16_t I, J;
	uint16_t PC, SP, EX, IA;
};

extern uint16_t clock_time;
extern struct register_file registers;
extern uint16_t memory[0x10000];

void sim_init(void);
uint16_t sim_step(void);
void sim_run(void);
void CATCH_FIRE(void);

#endif
