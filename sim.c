#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/signal.h>
#include <ncurses.h>
#include "core.h"
#include "dcpu16.h"
#include "hardware_host.h"
#include "lem1802.h"
#include "generic_clock.h"
#include "../jpd16a/disasm.h" // TODO don't do this

static void deinit(void);
static void dump_state(void);
static void dump_disassembly(void);
static int load_bin(const char *filename);
static void init_signal_handlers(void);
static void sigint_handler(int sig);

sig_atomic_t run, quit;

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

	sim_init();
	if (load_bin(corefilename) != 0)
		exit(1);

	attach_hardware_builtin();
	//attach_hardware_module("./lem1802");
	for (i = 0; i < 3; i++) {
		attach_hardware_module("./generic_clock");
	}
	//attach_hardware_module("./sped3");
	sim_start();

	initscr();
	atexit(deinit);
	if (dbg == 0) {
		run_dcpu16();
	} else {
		init_signal_handlers();

		run = 0;
		quit = 0;
		while (quit == 0) {
			int cmd;
			dump_state();
			if (run) {
				usleep(100000);
				sim_step();
			} else {
				cmd = getch();
				if (cmd == 'Q') {
					quit = 1;
				} else if (cmd == 'r') {
					run = 1;
				} else {
					sim_step();
				}
			}
		}
	}
	endwin();

	return 0;
}

void deinit(void)
{
	hardware_deinit();
	endwin();
}

void dump_state(void)
{
	int i, inst_len;
	uint16_t addr;
	char buf[32];

	mvprintw(0,  0, "Clock: 0x%.4x", clock_time);

	mvprintw(1,  0, "+------------+");
	mvprintw(2,  0, "| A:  0x%.4x |", registers->A);
	mvprintw(3,  0, "| B:  0x%.4x |", registers->B);
	mvprintw(4,  0, "| C:  0x%.4x |", registers->C);
	mvprintw(5,  0, "| X:  0x%.4x |", registers->X);
	mvprintw(6,  0, "| Y:  0x%.4x |", registers->Y);
	mvprintw(7,  0, "| Z:  0x%.4x |", registers->Z);
	mvprintw(8,  0, "| I:  0x%.4x |", registers->I);
	mvprintw(9,  0, "| J:  0x%.4x |", registers->J);
	mvprintw(10, 0, "| PC: 0x%.4x |", registers->PC);
	mvprintw(11, 0, "| SP: 0x%.4x |", registers->SP);
	mvprintw(12, 0, "| EX: 0x%.4x |", registers->EX);
	mvprintw(13, 0, "| IA: 0x%.4x |", registers->IA);
	mvprintw(14, 0, "+------------+");

	dump_disassembly();

	mvprintw(1, 80, "+----------------+");
	mvprintw(2, 80, "| NxtInt:   0x%.2x |", interrupts->first);
	mvprintw(3, 80, "| CurInt:   0x%.2x |", interrupts->last);
	mvprintw(4, 80, "| QueInt:   0x%.2x |",
			(interrupts->last + 256 - interrupts->first) % 256);
	mvprintw(5, 80, "| HwAtch: 0x%.4x |", hardware_get_attached());
	mvprintw(6, 80, "+----------------+");

	mvprintw(1, 99, "+--------+");
	for (i = 0; i < 12 && i+registers->SP < 0x10000; i++) {
		mvprintw(2+i, 99, "| 0x%.4x |", memory[registers->SP + i]);
	}
	mvprintw(2+i, 99, "+--------+");
	for (i++; i <= 12; i++) {
		mvprintw(2+i, 99, "          ");
	}

	for (i = 0; i < 10; i++)
		mvprintw(16 + i, 0,
				"%.4x: %.4x %.4x %.4x %.4x %.4x %.4x %.4x %.4x",
				i * 8, memory[i * 8], memory[i * 8 + 1],
				memory[i * 8 + 2], memory[i * 8 + 3],
				memory[i * 8 + 4], memory[i * 8 + 5],
				memory[i * 8 + 6], memory[i * 8 + 7]);

	attron(A_BOLD);
	inst_len = decode_inst(&memory[registers->PC], buf);
	for (i = 0; i < inst_len; i++) {
		addr = registers->PC + i;
		mvprintw(16 + (addr / 8), 6 + 5 * (addr % 8), "%.4x", memory[addr]);
	}
	attroff(A_BOLD);

	refresh();
}

void dump_disassembly(void)
{
	int i, pc_index, inst_len, offset;
	uint16_t addr;
	static uint16_t addr_buf[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
	char buf[32];
	int disasm_x = 15;

	pc_index = 0;
	for (i = 0; i < 12; i++) {
		if (addr_buf[i] == registers->PC) {
			pc_index = i;
			break;
		}
	}
	offset = 0;
	if (pc_index > 5)
		offset = pc_index - 5;

	mvprintw(1, disasm_x, "+---------------------------+");
	for (i = 0; (i + offset) < pc_index; i++) {
		decode_inst(&memory[addr_buf[i + offset]], buf);
		mvprintw(i + 2, disasm_x, "|                           |");
		mvprintw(i + 2, disasm_x, "| %.4x %s", addr_buf[i + offset], buf);
		addr_buf[i] = addr_buf[i + offset];
	}
	for (addr = registers->PC; i < 12; i++, addr += inst_len) {
		inst_len = decode_inst(&memory[addr], buf);
		mvprintw(i + 2, disasm_x, "|                           |");
		if (addr == registers->PC)
			attron(A_BOLD);
		mvprintw(i + 2, disasm_x + 2,   "%.4x %s", addr, buf);
		if (addr == registers->PC)
			attroff(A_BOLD);
		addr_buf[i] = addr;
	}
	mvprintw(14, disasm_x, "+---------------------------+");
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

void init_signal_handlers(void)
{
	struct sigaction sa;

	sa.sa_handler = sigint_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
}

void sigint_handler(int sig)
{
	(void)sig;
	run = 0;
}
