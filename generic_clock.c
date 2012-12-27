#include <stdlib.h>
#include <time.h>
//#include "core.h"
#include "hardware.h"
#include "hardware_host.h"

static int clk_scale = 1;
static int clk_enabled = 0;
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
	uint16_t b = read_register(REG_B);

	switch(read_register(REG_A)) {
	case 0:
		if (b) {
			clk_enabled = 1;
			clock_gettime(CLOCK_REALTIME, &t0);
			clk_scale = b;
		} else {
			clk_enabled = 0;
		}
		break;
	case 1:
		write_register(REG_C, ticks);
		break;
	case 2:
		if (b) {
			int_enabled = 1;
			int_msg = b;
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

		if (int_enabled && (old_ticks != ticks))
		// TODO if more than one tick passed, trigger more than once? or maybe that should be impossible, since the nominal clock rate is 100 kHz and it would only be a problem on massively scaled down debug mode.
			raise_interrupt(int_msg);
	}
}
