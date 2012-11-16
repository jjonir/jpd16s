#include <stdint.h>
#include <time.h>
#include "hardware.h"
#include "m35fd.h"

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

struct hardware fd = {
	{0x4fd5, 0x24c5},
	0x000b,
	{0x1eb3, 0x7e91},
	&fd_init,
	&fd_interrupt,
	&fd_step
};

struct hardware *m35fd(uint16_t *buf)
{
	disk = buf;
	return &fd;
}

void fd_init(void)
{
}

int fd_interrupt(void)
{
	uint16_t x = read_register(REG_X);

	switch (read_register(REG_A)) {
	case 0:
		write_register(REG_B, state);
		write_register(REG_C, error);
		break;
	case 1:
		if (x) {
			int_enabled = 1;
			int_msg = x;
		} else {
			int_enabled = 0;
		}
		break;
	case 2:
		if (start_read(read_register(REG_X), read_register(REG_Y)) == 0)
			write_register(REG_B, 1);
		else
			write_register(REG_B, 0);
		break;
	case 3:
		if (start_write(read_register(REG_X), read_register(REG_Y)) == 0)
			write_register(REG_B, 1);
		else
			write_register(REG_B, 0);
		break;
	default:
		break;
	}

	return 0;
}

void fd_step(void)
{
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
			if (((clock() - read_start) * 1000000 / CLOCKS_PER_SEC) > 16287)
				write_memory(ram_addr, SECTOR_LEN, &disk[disk_sector * SECTOR_LEN]);
		}
	} else if (write_in_progress) {
		if (!writing) {
			writing = 1;
			write_start = clock();
		} else {
			if (((clock() - write_start) * 1000000 / CLOCKS_PER_SEC) > 16287)
				read_memory(ram_addr, SECTOR_LEN, &disk[disk_sector * SECTOR_LEN]);
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
