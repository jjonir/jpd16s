#ifndef _CORE_H
#define _CORE_H

struct register_file{
	uint16_t A, B, C;
	uint16_t X, Y, Z;
	uint16_t I, J;
	uint16_t PC, SP, EX, IA;
};

extern struct register_file registers;
extern uint16_t memory[0x10000];

void run_dcpu16(void);

#endif
