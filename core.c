#include <stdlib.h>
#include <stdint.h>
#include "core.h"
#include "hardware.h"
#include "interrupts.h"

enum {
	SPC = 0x00,
	SET,
	ADD, SUB, MUL, MLI, DIV, DVI, MOD, MDI,
	AND, BOR, XOR, SHR, ASR, SHL,
	IFB, IFC, IFE, IFN, IFG, IFA, IFL, IFU,
	ADX = 0x1A, SBX,
	STI = 0x1E, STD
};

const uint16_t inst_clocks[0x20] = {
	0, 1, 2, 2, 2, 2, 3, 3, 3, 3, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 3, 3, 0, 0, 2, 2
};

enum {
	RES = 0x00,
	JSR,
	INT = 0x08, IAG, IAS, RFI, IAQ,
	HWN = 0x10, HWQ, HWI
};

const uint16_t spc_inst_clocks[0x20] = {
	0, 3, 0, 0, 0, 0, 0, 0, 4, 1, 1, 3, 2, 0, 0, 0,
	2, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

uint16_t clock_time = 0;

struct register_file registers;
uint16_t *register_array = (uint16_t *)&registers;
uint16_t memory[0x10000];

struct hardware *hardware[0x10000];


static void clock_tick(uint16_t cycles);
static uint16_t *next_word_addr(void);
static uint16_t next_word(void);
static uint16_t *decode_lvalue(uint16_t val);
static uint16_t decode_rvalue(uint16_t val);
static void skip(void);
static void ex_spc(uint16_t inst);
static void ex(void);


void clock_tick(uint16_t cycles)
{
	// TODO something else
	clock_time += cycles;
}

uint16_t *next_word_addr(void)
{
	clock_tick(1);
	return &memory[registers.PC++];
}

uint16_t next_word(void)
{
	return *next_word_addr();
}

/*
 * Decode the first argument. The specification calls this 'b'.
 * It is 5 bits wide and cannot be a literal.
 */
uint16_t *decode_lvalue(uint16_t val)
{
	if (val < 0x08)
		return &register_array[val];
	else if (val < 0x10)
		return &memory[register_array[val - 0x08]];
	else if (val < 0x18)
		return &memory[register_array[val - 0x10] + next_word()];
	else
		switch (val) {
		case 0x18: return &memory[--registers.SP];
		case 0x19: return &memory[registers.SP];
		case 0x1A: return &memory[registers.SP + next_word()];
		case 0x1B: return &registers.SP;
		case 0x1C: return &registers.PC;
		case 0x1D: return &registers.EX;
		case 0x1E: return &memory[next_word()];
		case 0x1F: return next_word_addr();
		default:
			return NULL;
			break;
		}
}

/*
 * Decode the second argument. The specification calls this 'a'.
 * It is 6 bits wide and can be a literal.
 */
uint16_t decode_rvalue(uint16_t val)
{
	if (val == 0x18)
		return memory[registers.SP++];
	else if (val < 0x20)
		return *decode_lvalue(val);
	else
		return val - 0x21;
}

void skip(void)
{
	uint16_t inst, op, val_a, val_b;

	do {
		clock_tick(1);
		inst = next_word();
		op = inst & 0x1F;
		val_b = (inst >> 5) & 0x1F;
		val_a = (inst >> 10) & 0x3F;

		if (((val_a >= 0x10) && (val_a <= 0x17)) ||
				(val_a == 0x1A) || (val_a == 0x1E) || (val_a == 0x1F))
			registers.PC++; // TODO spec suggests that this doesn't take a tick

		if (((val_b >= 0x10) && (val_b <= 0x17)) ||
				(val_b == 0x1A) || (val_b == 0x1E) || (val_b == 0x1F))
			registers.PC++; // TODO spec suggests that this doesn't take a tick
	} while ((inst >= 0x10) && (inst <= 0x17));
}

void ex_spc(uint16_t inst)
{
	uint16_t op, rvalue, *lvalue;

	op = (inst >> 5) & 0x1F;

	clock_tick(spc_inst_clocks[op]);
	if (op == RES) {
		// TODO unspecified, catch fire or fail silently?
		return;
	}

	if (((inst >> 10) & 0x3F) < 0x20) {
		lvalue = decode_lvalue((inst >> 10) & 0x1F);
		rvalue = *lvalue;
	} else {
		lvalue = NULL;
		rvalue = decode_rvalue((inst >> 10) & 0x3F);
	}

	switch (op) {
	case JSR:
		memory[--registers.SP] = registers.PC;
		registers.PC = rvalue;
		break;
	case INT:
		queue_interrupt(rvalue);
		break;
	case IAG:
		if (lvalue != NULL) // TODO spec says writing to literals fails silently, I don't like it
			*lvalue = registers.IA;
		break;
	case IAS:
		registers.IA = rvalue;
		break;
	case RFI:
		// TODO I think the argument is ignored, make sure that's the case
		interrupts_enabled = 1; // (disable queueing)
		registers.A = memory[registers.SP++];
		registers.PC = memory[registers.SP++];
		break;
	case IAQ:
		if (rvalue == 0)
			interrupts_enabled = 1; // (disable queueing)
		else
			interrupts_enabled = 0; // (enable queueing)
		break;
	case HWN:
		if (lvalue != NULL) // TODO spec says writing to literals fails silently, I don't like it
			// TODO enumerate hardware into *lvalue
		break;
	case HWQ:
		// TODO get info about hardware at location rvalue
		break;
	case HWI:
		// TODO block for some time, using the return value of interrupt()
		hardware[rvalue]->interrupt();
		break;
	default:
		// TODO unspecified, catch fire or fail silently?
		break;
	}
}

void ex(void)
{
	uint16_t inst, op, *lvalue, rvalue, temp_EX;

	inst = next_word();
	op = inst & 0x1F;

	if (op == SPC) {
		ex_spc(inst);
		return;
	}
	clock_tick(inst_clocks[op] - 1);

	rvalue = decode_rvalue((inst >> 10) & 0x3F);
	lvalue = decode_lvalue((inst >> 5) & 0x1F);

	switch (op) {
	case SET:
		*lvalue = rvalue;
		break;
	case ADD:
		registers.EX = (((uint32_t)*lvalue + (uint32_t)rvalue) >> 16) & 0xFFFF;
		*lvalue += rvalue;
		break;
	case SUB:
		registers.EX = (((int32_t)*lvalue - (int32_t)rvalue) >> 16) & 0xFFFF;
		*lvalue -= rvalue;
		break;
	case MUL:
		registers.EX = (((uint32_t)*lvalue * (uint32_t)rvalue) >> 16) & 0xFFFF;
		*lvalue *= rvalue;
		break;
	case MLI:
		registers.EX = (((int32_t)*lvalue * (int32_t)rvalue) >> 16) & 0xFFFF;
		*(int16_t *)lvalue *= (int16_t)rvalue;
		break;
	case DIV:
		if (rvalue == 0) {
			registers.EX = 0;
			*lvalue = 0;
		} else {
			registers.EX = (((uint32_t)*lvalue << 16) / (uint32_t)rvalue) & 0xFFFF;
			*lvalue /= rvalue;
		}
		break;
	case DVI:
		if (rvalue == 0) {
			registers.EX = 0;
			*lvalue = 0;
		} else {
			registers.EX = (((int32_t)*lvalue << 16) / (int32_t)rvalue) & 0xFFFF;
			*(int16_t *)lvalue /= (int16_t)rvalue;
		}
		break;
	case MOD:
		if (rvalue == 0)
			*lvalue = 0;
		else
			*lvalue %= rvalue;
		break;
	case MDI:
		if (rvalue == 0)
			*lvalue = 0;
		else
			*(int16_t *)lvalue %= (int16_t)rvalue;
		break;
	case AND:
		*lvalue &= rvalue;
		break;
	case BOR:
		*lvalue |= rvalue;
		break;
	case XOR:
		*lvalue ^= rvalue;
		break;
	case SHR:
		registers.EX = (((uint32_t)*lvalue << 16) >> rvalue) & 0xFFFF;
		*lvalue >>= rvalue;
		break;
	case ASR:
		registers.EX = (((uint32_t)*lvalue << 16) >> rvalue) & 0xFFFF;
		*(int16_t *)lvalue >>= rvalue;
		break;
	case SHL:
		registers.EX = (((uint32_t)*lvalue << rvalue) >> 16) & 0xFFFF;
		*lvalue <<= rvalue;
		break;
	case IFB:
		if ((*lvalue & rvalue) == 0)
			skip();
		break;
	case IFC:
		if ((*lvalue & rvalue) != 0)
			skip();
		break;
	case IFE:
		if (*lvalue != rvalue)
			skip();
		break;
	case IFN:
		if (*lvalue == rvalue)
			skip();
		break;
	case IFG:
		if (*lvalue <= rvalue)
			skip();
		break;
	case IFA:
		if (*(int16_t *)lvalue <= (int16_t)rvalue)
			skip();
		break;
	case IFL:
		if (*lvalue >= rvalue)
			skip();
		break;
	case IFU:
		if (*(int16_t *)lvalue >= (int16_t)rvalue)
			skip();
		break;
	case ADX:
		temp_EX = (((uint32_t)*lvalue + (uint32_t)rvalue +
							(uint32_t)registers.EX) >> 16) & 0xFFFF;
		*lvalue += (rvalue + registers.EX);
		registers.EX = temp_EX;
		break;
	case SBX:
		temp_EX = (((int32_t)*lvalue - ((int32_t)rvalue +
							(int32_t)registers.EX)) >> 16) & 0xFFFF;
		*lvalue -= (rvalue + registers.EX);
		registers.EX = temp_EX;
		break;
	case STI:
		*lvalue = rvalue;
		registers.I += 1;
		registers.J += 1;
		break;
	case STD:
		*lvalue = rvalue;
		registers.I -= 1;
		registers.J -= 1;
		break;
	default:
		// TODO unspecified, catch fire or fail silently?
		break;
	}
}

void sim_init(void)
{
}

uint16_t sim_step(void)
{
	int clock_before = clock_time;

	ex();
	if (interrupts_enabled)
		trigger_next_queued_interrupt();
	// TODO step all connected devices
	hardware[0]->step(); // hardwired LEM1802
	hardware[1]->step(); // hardwired generic clock

	return clock_time - clock_before;
}

void run_dcpu16(void)
{
	while (1) {
		sim_step();
	}
}

// TODO make a more accurate hcf routine
void CATCH_FIRE(void)
{
	exit(1);
}
