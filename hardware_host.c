#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include "hardware.h"
#include "hardware_host.h"
#include "lem1802.h"
#include "generic_clock.h"

struct hardware hardware[0x10000];

static uint16_t attached = 0; // TODO if exactly 0x10000 hw devices are attached, it will wrap - should support exactly 0x10000 and hcf if more are attached
static int dbgline = 27;
static int dbgrow = 0;
#define DBGPRINT(...) \
do { \
	mvprintw(dbgline++, dbgrow, __VA_ARGS__); \
	if (dbgline > 46) { dbgline = 27; dbgrow += 30; } \
	refresh(); \
} while (0);

static void readline(int fd, char *buf, int max);
static int readline_async(int fd, char *buf, int max);
static void module_cmd(struct hw_module *hw, const char *cmd, int len);
static void module_waitreply(struct hw_module *hw);
static void hardware_step(uint16_t where);

void attach_hardware_builtin(void)
{
	uint16_t where;

	where = attached++;
	hardware[where].type = HARDWARE_BUILTIN;
	hardware[where].hw.builtin = lem1802(2, 46); // TODO 1, 1 if not debug?

	where = attached++;
	hardware[where].type = HARDWARE_BUILTIN;
	hardware[where].hw.builtin = generic_clock();
}

int attach_hardware_module(const char *name)
{
	uint16_t where;
	int tx_fds[2], rx_fds[2];
	char buf[32];

	pipe(tx_fds);
	pipe(rx_fds);

	if (fork() == 0) {
		// connect tx_fds receiving end to stdin
		close(0);
		dup(tx_fds[0]);
		close(tx_fds[1]);

		// connect rx_fds sending end to stdout
		close(1);
		dup(rx_fds[1]);
		close(rx_fds[0]);

		execlp(name, name, NULL);

		fprintf(stderr, "error exec\'ing %s\n", name);
		perror("execlp");
	}

	close(tx_fds[0]);
	close(rx_fds[1]);

	where = attached++;
	hardware[where].type = HARDWARE_MODULE;
	hardware[where].hw.module = (struct hw_module *)malloc(sizeof(struct hw_module));
	hardware[where].hw.module->tx = tx_fds[1];
	hardware[where].hw.module->rx = rx_fds[0];

	readline(rx_fds[0], buf, 32);
	readline(rx_fds[0], buf, 32);

	return 0;
}

void readline(int fd, char *buf, int max)
{
	int i = 0;

	memset(buf, 0, max);
	do {
		read(fd, &buf[i], 1);
		i++;
	} while ((i < max) && (buf[i - 1] != '\n'));
}

int readline_async(int fd, char *buf, int max)
{
	int fl = fcntl(fd, F_GETFL);

	fcntl(fd, F_SETFL, fl | O_NONBLOCK);
	if (1 != read(fd, &buf[0], 1))
		return 0;
	fcntl(fd, F_SETFL, fl);

	if ((max > 1) && (buf[0] != '\n'))
		readline(fd, buf + 1, max - 1);

	return strlen(buf);
}

void module_cmd(struct hw_module *hw, const char *cmd, int len)
{
	write(hw->tx, cmd, len);
	DBGPRINT("to module: %s", cmd);
	module_waitreply(hw);
}

void module_waitreply(struct hw_module *hw)
{
	uint16_t arg1, arg2;
	char buf[32], reply[32];

	do {
		readline(hw->rx, buf, 32);
		DBGPRINT("from module: %s", buf);
		if (strncmp(buf, "reg r", 5) == 0) {
			sscanf(buf, "reg r %hu", &arg1);
			arg1 = read_register(arg1);
			sprintf(reply, "%i\n", arg1);
			write(hw->tx, reply, strlen(reply));
			DBGPRINT("to module: %s", reply);
		} else if (strncmp(buf, "reg w", 5) == 0) {
			sscanf(buf, "reg w %hu %hu", &arg1, &arg2);
			write_register(arg1, arg2);
		} else if (strncmp(buf, "mem r", 5) == 0) {
			sscanf(buf, "mem r %hu %hu", &arg1, &arg2);
		} else if (strncmp(buf, "mem w", 5) == 0) {
			sscanf(buf, "mem w %hu %hu", &arg1, &arg2);
		}
	} while (strcmp(buf, "done\n") != 0);
}

void hardware_hwi(uint16_t where)
{
	if (hardware[where].type != HARDWARE_NONE) {
		switch(hardware[where].type) {
		case HARDWARE_BUILTIN:
			hardware[where].hw.builtin->interrupt();
			break;
		case HARDWARE_MODULE:
			module_cmd(hardware[where].hw.module, "hwi\n", 4);
			break;
		default:
			break;
		}
	}
}

void hardware_hwq(uint16_t where)
{
	switch (hardware[where].type) {
	case HARDWARE_NONE:
		break;
	case HARDWARE_BUILTIN:
		write_register(REG_A, hardware[where].hw.builtin->id & 0xFFFF);
		write_register(REG_B, hardware[where].hw.builtin->id >> 16);
		write_register(REG_C, hardware[where].hw.builtin->version);
		write_register(REG_X, hardware[where].hw.builtin->mfg & 0xFFFF);
		write_register(REG_Y, hardware[where].hw.builtin->mfg >> 16);
		break;
	case HARDWARE_MODULE:
		module_cmd(hardware[where].hw.module, "hwq\n", 4);
		break;
	default:
		break;
	}
}

void hardware_step_all(void)
{
	int i;

	for (i = 0; i < 0x10000; i++)
		if (hardware[i].type != HARDWARE_NONE)
			hardware_step(i);
}

void hardware_step(uint16_t where)
{
	uint16_t arg;
	char buf[32];

	switch(hardware[where].type) {
	case HARDWARE_BUILTIN:
		hardware[where].hw.builtin->step();
		break;
	case HARDWARE_MODULE:
		if (readline_async(hardware[where].hw.module->rx, buf, 32)) {
			if (strncmp(buf, "int", 3) == 0) {
				DBGPRINT("from module: %s", buf);
				sscanf(buf, "int %hu", &arg);
				raise_interrupt(arg);
			} else {
				// TODO bad async msg, shutdown module?
			}
		}
		break;
	default:
		break;
	}
}

uint16_t hardware_get_attached(void)
{
	return attached;
}
