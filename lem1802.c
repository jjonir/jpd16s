#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <curses.h>
#include "core.h"
#include "hardware.h"
#include "hardware_host.h"

#define VRAM_LEN 384
#define VRAM_ROWS 12
#define VRAM_COLS 32

#define FONT_LEN 256
#define PAL_LEN 16

static int vram_top = 2;
static int vram_left = 46;

static int connected = 0;
static uint16_t vram;
static uint16_t font[FONT_LEN]; // TODO define default font
static int custom_font = 0;
static uint16_t font_ram;
static uint16_t pal[PAL_LEN]; // TODO define default pal
static int custom_pal = 0;
static uint16_t pal_ram;
static uint16_t border_col;
static struct timespec tp_prev;

static void lem_init(void);
static int lem_interrupt(void);
static void lem_step(void);

struct hw_builtin lem_builtin = {
	0x7349f615,
	0x1802,
	0x1c6c8b36,
	&lem_init,
	&lem_interrupt,
	&lem_step
};

struct hw_builtin *lem1802(int top, int left)
{
	struct hw_builtin *lem;

	vram_top = top;
	vram_left = left;

	lem = (struct hw_builtin *)malloc(sizeof(struct hw_builtin));
	*lem = lem_builtin;
	return lem;
}

#ifdef BUILD_MODULE
struct hw_builtin *get_hw(void)
{
	return lem1802(1, 1);
}
#endif

void lem_init(void)
{
	clock_gettime(CLOCK_REALTIME, &tp_prev);
}

int lem_interrupt(void)
{
	uint16_t cycles = 0;
	uint16_t b = registers->B;

	switch (registers->A) {
	case 0:
		if (b) {
			if (connected == 0) {
				clock_gettime(CLOCK_REALTIME, &tp_prev);
				// TODO don't start up for ~1s
			}
			connected = 1;
			vram = b;
		} else {
			connected = 0;
		}
		break;
	case 1:
		if (b) {
			custom_font = 1;
			font_ram = b;
		} else {
			custom_font = 1;
		}
		break;
	case 2:
		if (b) {
			custom_pal = 1;
			pal_ram = b;
		} else {
			custom_pal = 1;
		}
		break;
	case 3:
		border_col = b;
		break;
	case 4:
		memcpy(&memory[b], font, FONT_LEN);
		cycles = 256;
		break;
	case 5:
		memcpy(&memory[b], pal, PAL_LEN);
		cycles = 16;
		break;
	default:
		break;
	}

	return cycles;
}

void lem_step(void)
{
	uint16_t row, col, index;
	char row_buf[33] = "                                ";
	uint16_t vram_buf[384];
	struct timespec tp_cur;
	uint64_t ns;

	clock_gettime(CLOCK_REALTIME, &tp_cur);
	ns = (tp_cur.tv_sec - tp_prev.tv_sec) * 1000000000;
	ns += (tp_cur.tv_nsec - tp_prev.tv_nsec);
	if ((ns * 60 / 1000000000) == 0)
		return;

	mvprintw(vram_top - 1, vram_left - 1, "+--------------------------------+");

	if (connected) {
		memcpy(vram_buf, &memory[vram], VRAM_LEN);

		if (custom_font)
			return; // TODO something else
		for (row = 0; row < VRAM_ROWS; row++) {
			for (col = 0; col < VRAM_COLS; col++) {
				index = row * VRAM_COLS + col;
				row_buf[col] = (char)vram_buf[index];
				if (row_buf[col] == 0)
					row_buf[col] = ' ';
			}
			row_buf[col] = 0;
			mvprintw(vram_top + row, vram_left - 1, "|%s|", row_buf);
		}
	} else {
		for (row = 0; row < VRAM_ROWS; row++) {
			mvprintw(vram_top + row, vram_left - 1, "|%s|", row_buf);
		}
	}

	mvprintw(vram_top + 12, vram_left - 1, "+--------------------------------+");
	refresh();
}
