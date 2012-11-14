#include <string.h>
#include <stdint.h>
#include <curses.h>
#include "hardware.h"
#include "core.h"

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

static void lem_init(void);
static uint16_t lem_interrupt(void);
static void lem_step(void);

struct hardware lem = {
	{0x7349, 0xf615},
	0x1802,
	{0x1c6c, 0x8b36},
	&lem_init,
	&lem_interrupt,
	&lem_step
};

struct hardware *lem1802(int top, int left)
{
	vram_top = top;
	vram_left = left;
	return &lem;
}

void lem_init(void)
{
}

uint16_t lem_interrupt(void)
{
	uint16_t cycles = 0;

	switch (registers.A) {
	case 0:
		if (registers.B) {
			if (connected == 0) {
				// TODO don't start up for ~1s
			}
			connected = 1;
			vram = registers.B;
		} else {
			connected = 0;
		}
		break;
	case 1:
		if (registers.B) {
			custom_font = 1;
			font_ram = registers.B;
		} else {
			custom_font = 1;
		}
		break;
	case 2:
		if (registers.B) {
			custom_pal = 1;
			pal_ram = registers.B;
		} else {
			custom_pal = 1;
		}
		break;
	case 3:
		border_col = registers.B;
		break;
	case 4:
		memcpy(&memory[registers.B], font, FONT_LEN);
		cycles = 256;
		break;
	case 5:
		memcpy(&memory[registers.B], pal, PAL_LEN);
		cycles = 16;
	default:
		break;
	}

	return cycles;
}

void lem_step(void)
{
	uint16_t row, col, index;
	char row_buf[33] = "                                ";

	mvprintw(vram_top - 1, vram_left - 1, "+--------------------------------+");

	if (custom_font)
		return; // TODO something else
	for (row = 0; row < VRAM_ROWS; row++) {
		if (connected){
			for (col = 0; col < VRAM_COLS; col++) {
				index = row * VRAM_COLS + col;
				row_buf[col] = (char)memory[vram + index];
				if (row_buf[col] == 0)
					row_buf[col] = ' ';
			}
			row_buf[col] = 0;
		}
		mvprintw(vram_top + row, vram_left - 1, "|%s|", row_buf);
	}

	mvprintw(vram_top + 12, vram_left - 1, "+--------------------------------+");
	refresh();
}
