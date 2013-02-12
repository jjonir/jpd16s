#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include "dcpu16.h"

uint16_t *memory;
struct register_file *registers;
struct interrupts *interrupts;

sig_atomic_t int_requested, hwq_requested;

static void get_mem(void);
static void release_mem(void);
static void sigusr_handler(int n);

int init_module(void)
{
	struct sigaction sa;

	sa.sa_handler = sigusr_handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGUSR1, &sa, NULL) == -1) {
		perror("sigaction");
		return -1;
	}
	if (sigaction(SIGUSR2, &sa, NULL) == -1) {
		perror("sigaction");
		return -1;
	}

	get_mem();

	memory[0xFFFF] = 0; // confirm init complete TODO set to some other value if failure happens

	return 0;
}

int shutdown_module(void)
{
	release_mem();
	return 0;
}

void get_mem(void)
{
	int shm_fd;

	if ((shm_fd = shm_open("/dcpu16s-mem-shm", O_RDWR, 0644)) == -1) {
		perror("module: shm_open mem");
		exit(1);
	}
	if ((memory = mmap(0, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
			shm_fd, 0)) == MAP_FAILED) {
		perror("module: mmap mem");
		exit(1);
	}

	if ((shm_fd = shm_open("/dcpu16s-reg-shm", O_RDWR, 0644)) == -1) {
		perror("module: shm_open reg");
		exit(1);
	}
	if ((registers = mmap(0, REG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
			shm_fd, 0)) == MAP_FAILED) {
		perror("module: mmap reg");
		exit(1);
	}

	if ((shm_fd = shm_open("/dcpu16s-int-shm", O_RDWR, 0644)) == -1) {
		perror("module: shm_open int");
		exit(1);
	}
	if ((interrupts = mmap(0, INT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
			shm_fd, 0)) == MAP_FAILED) {
		perror("module: mmap int");
		exit(1);
	}
}

void release_mem(void)
{
	if (munmap(memory, MEM_SIZE) == -1) {
		perror("module: munmap memory");
	}
	if (munmap(registers, REG_SIZE) == -1) {
		perror("module: munmap registers");
	}
	if (munmap(interrupts, INT_SIZE) == -1) {
		perror("module: munmap interrupts:");
	}
}

void hwq(uint32_t id, uint16_t ver, uint32_t mfg)
{
	registers->A = id & 0xFFFF;
	registers->B = id >> 16;
	registers->C = ver;
	registers->X = mfg & 0xFFFF;
	registers->Y = mfg >> 16;
}

void sigusr_handler(int n)
{
	if (n == SIGUSR1) {
		int_requested = 1;
	} else if (n == SIGUSR2) {
		hwq_requested = 1;
	}
}
