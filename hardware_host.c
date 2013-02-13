#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "dcpu16.h"
#include "hardware_host.h"
#include "lem1802.h"
#include "generic_clock.h"
#include "sped3.h"

struct hardware hardware[0x10000];

static uint16_t attached = 0; // TODO if exactly 0x10000 hw devices are attached, it will wrap - should support exactly 0x10000 and hcf if more are attached

static void hardware_step(uint16_t where);

void attach_hardware_builtin(void)
{
	uint16_t where;

	where = attached++;
	hardware[where].type = HARDWARE_BUILTIN;
	hardware[where].hw.builtin = lem1802(2, 46); // TODO 1, 1 if not debug?
	hardware[where].hw.builtin->init();

/*
	where = attached++;
	hardware[where].type = HARDWARE_BUILTIN;
	hardware[where].hw.builtin = generic_clock();
	hardware[where].hw.builtin->init();

	where = attached++;
	hardware[where].type = HARDWARE_BUILTIN;
	hardware[where].hw.builtin = sped3();
	hardware[where].hw.builtin->init();
*/
}

int attach_hardware_module(const char *name)
{
	uint16_t where;
	pid_t pid;

	memory[0xFFFF] = 0xBEEF;

	if ((pid = fork()) == 0) {
		char logname[1024];
		int logfd;

		strcpy(logname, name);
		strcat(logname, ".log");
		logfd = fileno(fopen(logname, "w"));

		close(1);
		dup(logfd);
		close(2);
		dup(logfd);

		execlp(name, name, NULL);

		fprintf(stderr, "error exec\'ing %s\n", name);
		perror("execlp");
	}

	where = attached++;
	hardware[where].type = HARDWARE_MODULE;
	hardware[where].hw.module = (struct hw_module *)malloc(sizeof(struct hw_module));
	hardware[where].hw.module->pid = pid;

	while (memory[0xFFFF] != 0) {
		// spin waiting for module to confirm init complete
		// TODO use a semaphore or something, stack bottom is ok i guess for now
	}

	return 0;
}

void hardware_deinit(void)
{
	int i;
	int status;

	for (i = 0; i < attached; i++) {
		if (hardware[i].type == HARDWARE_MODULE) {
			kill(hardware[i].hw.module->pid, SIGINT);
			wait(&status);
			free(hardware[i].hw.module);
		}
	}
}

void hardware_hwi(uint16_t where)
{
	if (hardware[where].type != HARDWARE_NONE) {
		switch(hardware[where].type) {
		case HARDWARE_BUILTIN:
			hardware[where].hw.builtin->interrupt();
			break;
		case HARDWARE_MODULE:
			kill(hardware[where].hw.module->pid, SIGUSR1);
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
		registers->A = hardware[where].hw.builtin->id & 0xFFFF;
		registers->B = hardware[where].hw.builtin->id >> 16;
		registers->C = hardware[where].hw.builtin->version;
		registers->X = hardware[where].hw.builtin->mfg & 0xFFFF;
		registers->Y = hardware[where].hw.builtin->mfg >> 16;
		break;
	case HARDWARE_MODULE:
		kill(hardware[where].hw.module->pid, SIGUSR2);
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
	switch(hardware[where].type) {
	case HARDWARE_BUILTIN:
		hardware[where].hw.builtin->step();
		break;
	case HARDWARE_MODULE:
		break;
	default:
		break;
	}
}

uint16_t hardware_get_attached(void)
{
	return attached;
}
