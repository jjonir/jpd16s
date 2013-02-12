#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "dcpu16.h"
#include "m35fd.h"
#ifdef BUILD_MODULE
#include "hardware_module.h"
#else
#include "hardware_host.h"
#endif

#define HW_ID (0x4fd524c5)
#define HW_VER (0x000b)
#define HW_MFG (0x1eb37e91)

#define SECTOR_LEN 512
#define SECTORS_PER_TRACK 18
#define TRACK_LEN (SECTOR_LEN * SECTORS_PER_TRACK)

static uint16_t *disk;
static uint16_t state = 0;
static uint16_t error = 0;
static int int_enabled = 0;
static uint16_t int_msg;

static uint16_t track = 0, target_track;
static uint16_t ram_addr;
static uint32_t disk_sector;
static int read_in_progress = 0;
static int write_in_progress = 0;

static int seeking = 0;
static clock_t seek_start;
static int reading = 0;
static clock_t read_start;
static int writing = 0;
static clock_t write_start;

static void fd_init(void);
static int fd_interrupt(void);
static void fd_step(void);
static int start_read(uint16_t read_sector, uint16_t addr);
static int start_write(uint16_t write_sector, uint16_t addr);

#ifndef BUILD_MODULE
struct hw_builtin fd_builtin = {
	HW_ID,
	HW_VER,
	HW_MFG,
	&fd_init,
	&fd_interrupt,
	&fd_step
};

struct hw_builtin *m35fd(uint16_t *buf)
{
	struct hw_builtin *fd;

	disk = buf;

	fd = (struct hw_builtin *)malloc(sizeof(struct hw_builtin));
	*fd = fd_builtin;
	return fd;
}
#endif

#ifdef BUILD_MODULE
int main(int argc, char *argv[])
{
	init_module();
	fd_init();

	while (1) {
		fd_step();
		if (int_requested) {
			fprintf(stderr, "got hwi, A=0x%hx\n", registers->A);
			fd_interrupt();
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

void fd_init(void)
{
}

int fd_interrupt(void)
{
	switch (registers->A) {
	case 0:
		registers->B = state;
		registers->C = error;
		break;
	case 1:
		if (registers->X) {
			int_enabled = 1;
			int_msg = registers->X;
		} else {
			int_enabled = 0;
		}
		break;
	case 2:
		if (start_read(registers->X, registers->Y) == 0)
			registers->B = 1;
		else
			registers->B = 0;
		break;
	case 3:
		if (start_write(registers->X, registers->Y) == 0)
			registers->B = 1;
		else
			registers->B = 0;
		break;
	default:
		break;
	}

	return 0;
}

void fd_step(void)
{
	int i;

	if (track != target_track) {
		if (!seeking) {
			seeking = 1;
			seek_start = clock();
		} else {
			if (((clock() - seek_start) * 1000000 / CLOCKS_PER_SEC) > 2400) {
				if (target_track > track)
					track++;
				else
					track--;
				seeking = 0;
			}
		}
	} else if (read_in_progress) {
		if (!reading) {
			reading = 1;
			read_start = clock();
		} else {
			if (((clock() - read_start) * 1000000 / CLOCKS_PER_SEC) > 16287) {
				for (i = 0; i < SECTOR_LEN; i++) {
					memory[ram_addr + i] = disk[disk_sector * SECTOR_LEN + i];
				}
			}
		}
	} else if (write_in_progress) {
		if (!writing) {
			writing = 1;
			write_start = clock();
		} else {
			if (((clock() - write_start) * 1000000 / CLOCKS_PER_SEC) > 16287) {
				for (i = 0; i < SECTOR_LEN; i++) {
					disk[disk_sector * SECTOR_LEN + i] = memory[ram_addr + i];
				}
			}
		}
	}
}

static int start_read(uint16_t read_sector, uint16_t addr)
{
	if ((state == STATE_READY) || (state == STATE_READY_WP)) {
		disk_sector = read_sector;
		target_track = read_sector / 18;
		ram_addr = addr;
		read_in_progress = 1;
		state = STATE_BUSY;
		return 0;
	} else {
		return -1;
	}
}

static int start_write(uint16_t write_sector, uint16_t addr)
{
	if (state == STATE_READY) {
		disk_sector = write_sector;
		target_track = write_sector / 18;
		ram_addr = addr;
		write_in_progress = 1;
		state = STATE_BUSY;
		return 0;
	} else {
		return -1;
	}
}
