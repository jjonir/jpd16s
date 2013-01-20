#ifndef _CORE_H
#define _CORE_H

#include <stdint.h>

struct register_file {
	uint16_t A, B, C;
	uint16_t X, Y, Z;
	uint16_t I, J;
	uint16_t PC, SP, EX, IA;
};

extern struct register_file *registers;
extern uint16_t *register_array;
extern uint16_t *memory;
extern volatile uint16_t *int_vec;

#ifndef BUILD_MODULE
extern uint16_t clock_time;

void sim_init(void);
uint16_t sim_step(void);
void CATCH_FIRE(void);
#endif

#endif
