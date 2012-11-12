#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include "core.h"

static void dump_state(void);
static int load_bin(const char *filename);

int main(int argc, char *argv[])
{
	int i;
	int dbg = 0;
	char corefilename[1024] = "core.bin";

	for (i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "-d") == 0) ||
		    (strcmp(argv[i], "--debug") == 0))
			dbg = 1;
		else
			strncpy(corefilename, argv[i], 1023);
	}

	if (load_bin(corefilename) != 0)
		exit(1);

	if (dbg == 0) {
		run_dcpu16();
	} else {
		initscr();
		while (1) {
			dump_state();
			sim_step();
		}
		endwin();
	}

	return 0;
}

void dump_state(void)
{
	int i;

	mvprintw(0,  0, "|Clock Ticks: 0x%.4x", clock);

	mvprintw(1,  0, "+----------+");
	mvprintw(2,  0, "|A:  0x%.4x|", registers.A);
	mvprintw(3,  0, "|B:  0x%.4x|", registers.B);
	mvprintw(4,  0, "|C:  0x%.4x|", registers.C);
	mvprintw(5,  0, "|X:  0x%.4x|", registers.X);
	mvprintw(6,  0, "|Y:  0x%.4x|", registers.Y);
	mvprintw(7,  0, "|Z:  0x%.4x|", registers.Z);
	mvprintw(8,  0, "|I:  0x%.4x|", registers.I);
	mvprintw(9,  0, "|J:  0x%.4x|", registers.J);
	mvprintw(10, 0, "|PC: 0x%.4x|", registers.PC);
	mvprintw(11, 0, "|SP: 0x%.4x|", registers.SP);
	mvprintw(12, 0, "|EX: 0x%.4x|", registers.EX);
	mvprintw(13, 0, "|IA: 0x%.4x|", registers.IA);
	mvprintw(14, 0, "+-----------");

	for (i = 0; i < 30; i++)
		mvprintw(15 + i, 0,
				"%.4x: %.4x %.4x %.4x %.4x %.4x %.4x %.4x %.4x",
				i * 8, memory[i * 8], memory[i * 8 + 1],
				memory[i * 8 + 2], memory[i * 8 + 3],
				memory[i * 8 + 4], memory[i * 8 + 5],
				memory[i * 8 + 6], memory[i * 8 + 7]);

	refresh();
}

int load_bin(const char *filename)
{
	FILE *f;

	if ((f = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "error opening core file %s\n", filename);
		perror("fopen");
		return -1;
	}
	if (fread((void *)memory, 2, 0x10000, f) == 0) {
		fprintf(stderr, "core file %s unreadable or empty\n", filename);
		return -2;
	}
	fgetc(f);
	if (!feof(f)) {
		fprintf(stderr, "core file larger than the maximum 0x10000 words\n");
		return -3;
	}
	fclose(f);

	return 0;
}
