#include <stdlib.h>
#include <time.h>
#include "dcpu16.h"
#include "hardware_host.h"

static int clk_enabled = 0;
static int clk_scale;
static int int_enabled = 0;
static uint16_t int_msg;
static struct timespec t0;
static int ticks = 0;

static void clk_init(void);
static int clk_interrupt(void);
static void clk_step(void);

struct hw_builtin clk_builtin = {
	0x12d0b402,
	0x0001,
	0x00000000,
	&clk_init,
	&clk_interrupt,
	&clk_step
};

struct hw_builtin *generic_clock(void)
{
	struct hw_builtin *clk;

	clk = (struct hw_builtin *)malloc(sizeof(struct hw_builtin));
	*clk = clk_builtin;
	return clk;
}

#ifdef BUILD_MODULE
struct hw_builtin *get_hw(void)
{
	return generic_clock();
}
#endif

void clk_init(void)
{
}

int clk_interrupt(void)
{
	switch(registers->A) {
	case 0:
		if (registers->B) {
			clk_enabled = 1;
			clock_gettime(CLOCK_REALTIME, &t0);
			clk_scale = registers->B;
		} else {
			clk_enabled = 0;
		}
		break;
	case 1:
		registers->C = ticks;
		break;
	case 2:
		if (registers->B) {
			int_enabled = 1;
			int_msg = registers->B;
		} else {
			int_enabled = 0;
		}
		break;
	default:
		break;
	}

	return 0;
}

void clk_step(void)
{
	struct timespec tp;
	uint64_t ns;
	int old_ticks = ticks;

	if (clk_enabled) {
		clock_gettime(CLOCK_REALTIME, &tp);
		ns = (tp.tv_sec - t0.tv_sec) * 1000000000;
		ns += (tp.tv_nsec - t0.tv_nsec);
		ticks = ns * 60 / 1000000000 / clk_scale;

		// if somehow more than 1 tick passed, this will only trigger once
		if (int_enabled && (old_ticks != ticks)) {
			raise_interrupt(int_msg);
		}
	}
}
