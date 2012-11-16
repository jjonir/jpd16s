#include <time.h>
#include "core.h"
#include "hardware.h"

static int clk_scale = 1;
static int clk_enabled = 0;
static int int_enabled = 0;
static int int_msg;
static clock_t t0;
static int ticks = 0;

static void clk_init(void);
static int clk_interrupt(void);
static void clk_step(void);

struct hardware clk = {
	{0x12d0, 0xb402},
	0x0001,
	{0x0000, 0x0000},
	&clk_init,
	&clk_interrupt,
	&clk_step
};

struct hardware *generic_clock(void)
{
	return &clk;
}

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
			t0 = clock();
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
	int old_ticks = ticks;
	ticks = (clock() - t0) * 60 / CLOCKS_PER_SEC / clk_scale;
	if (old_ticks != ticks) // TODO if more than one tick passed, trigger more than once? or maybe that should be impossible, since the nominal clock rate is 100 MHz and it would only be a problem on massively scaled down debug mode.
		raise_interrupt(int_msg);
}
