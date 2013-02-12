#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <curses.h>
#include "dcpu16.h"

#ifdef BUILD_MODULE
#include "hardware_module.h"
#else
#include "hardware_host.h"
#endif

#define HW_ID (0x7349f615)
#define HW_VER (0x1802)
#define HW_MFG (0x1c6c8b36)

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

#ifndef BUILD_MODULE
struct hw_builtin lem_builtin = {
	HW_ID,
	HW_VER,
	HW_MFG,
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
#endif

#ifdef BUILD_MODULE
int main(int argc, char *argv[])
{
	init_module();
	lem_init();

	while (1) {
		lem_step();
		if (int_requested) {
			fprintf(stderr, "got hwi, A=0x%hx\n", registers->A);
			lem_interrupt();
			int_requested = 0;
		}
		if (hwq_requested) {
			fprintf(stderr, "got hwq\n");
			hwq(HW_ID, HW_VER, HW_MFG);
			hwq_requested = 0;
		}
	}

	shutdown_module();
	return 0;
}
#endif

void lem_init(void)
{
	clock_gettime(CLOCK_REALTIME, &tp_prev);
}

int lem_interrupt(void)
{
	uint16_t cycles = 0;
	int i;

	switch (registers->A) {
	case 0:
		if (registers->B) {
			if (connected == 0) {
				clock_gettime(CLOCK_REALTIME, &tp_prev);
				// TODO don't start up for ~1s
			}
			connected = 1;
			vram = registers->B;
		} else {
			connected = 0;
		}
		break;
	case 1:
		if (registers->B) {
			custom_font = 1;
			font_ram = registers->B;
		} else {
			custom_font = 1;
		}
		break;
	case 2:
		if (registers->B) {
			custom_pal = 1;
			pal_ram = registers->B;
		} else {
			custom_pal = 1;
		}
		break;
	case 3:
		border_col = registers->B;
		break;
	case 4:
		for (i = 0; i < FONT_LEN; i++) {
			memory[registers->B + i] = font[i];
		}
		cycles = 256;
		break;
	case 5:
		for (i = 0; i < PAL_LEN; i++) {
			memory[registers->B + i] = pal[i];
		}
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
	int i;

	clock_gettime(CLOCK_REALTIME, &tp_cur);
	ns = (tp_cur.tv_sec - tp_prev.tv_sec) * 1000000000;
	ns += (tp_cur.tv_nsec - tp_prev.tv_nsec);
	if ((ns * 60 / 1000000000) == 0)
		return;

	mvprintw(vram_top - 1, vram_left - 1, "+--------------------------------+");

	if (connected) {
		for (i = 0; i < VRAM_LEN; i++) {
			vram_buf[i] = memory[vram + i];
		}

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
			if (row == VRAM_ROWS / 2) {
				mvprintw(vram_top + row, vram_left - 1, "|     LEM1802 - not connected    |");
			} else {
				mvprintw(vram_top + row, vram_left - 1, "|%s|", row_buf);
			}
		}
	}

	mvprintw(vram_top + 12, vram_left - 1, "+--------------------------------+");
	refresh();
}
