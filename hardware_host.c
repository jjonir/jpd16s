#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <stdint.h>
#include "core.h"
#include "hardware.h"
#include "hardware_host.h"
#include "interrupts.h"
#include "lem1802.h"
#include "generic_clock.h"

struct hardware hardware[0x10000];

static uint16_t attached = 0; // TODO die if 0xFFFF attached and another is tried

static void hardware_step(uint16_t where);

void attach_hardware_builtin(void)
{
	uint16_t where;

	where = attached++;
	hardware[where].type = HARDWARE_BUILTIN;
	hardware[where].hw.builtin = lem1802(2, 46); // TODO 1, 1 if not debug?

#if 0
	where = attached++;
	hardware[where].type = HARDWARE_BUILTIN;
	hardware[where].hw.builtin = generic_clock();
#endif
}

int attach_hardware_module(const char *name)
{
	uint16_t where;
	pid_t pid;

	where = attached;

	registers->A = where;
	if ((pid = fork()) == 0) {
		execlp(name, name, NULL);

		fprintf(stderr, "error exec\'ing %s\n", name);
		perror("execlp");
	}
	while (registers->A != 0);

	attached++;
	hardware[where].type = HARDWARE_MODULE;
	hardware[where].hw.module = (struct hw_module *)malloc(sizeof(struct hw_module));
	hardware[where].hw.module->pid = pid;

	return 0;
}

void detach_all_hardware_modules(void)
{
	int i, status;
	for (i = 0; i < attached; i++) {
		if (hardware[i].type != HARDWARE_MODULE)
			continue;
		kill(hardware[i].hw.module->pid, SIGINT);
		wait(&status);
	}
}

void hardware_hwi(uint16_t where)
{
	switch(hardware[where].type) {
	case HARDWARE_NONE:
		break;
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

	for (i = 0; i < attached; i++)
		hardware_step(i);
}

void hardware_step(uint16_t where)
{
	struct sembuf sembuf;
	int int_sem = semget(ftok("jpd16s", 'i'), 1, 0644);

	switch(hardware[where].type) {
	case HARDWARE_NONE:
		break;
	case HARDWARE_BUILTIN:
		hardware[where].hw.builtin->step();
		break;
	case HARDWARE_MODULE:
		sembuf.sem_num = 0;
		sembuf.sem_op = -1;
		sembuf.sem_flg = 0;
		semop(int_sem, &sembuf, 1);
		if (int_vec[where]) {
			raise_interrupt(int_vec[where]);
			int_vec[where] = 0;
		}
		sembuf.sem_num = 0;
		sembuf.sem_op = 1;
		sembuf.sem_flg = 0;
		semop(int_sem, &sembuf, 1);
		break;
	default:
		break;
	}
}

uint16_t hardware_get_attached(void)
{
	return attached;
}

void raise_interrupt(uint16_t msg)
{
	queue_interrupt(msg);
}
