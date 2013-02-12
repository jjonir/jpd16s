#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include "dcpu16.h"

uint16_t *memory;
struct register_file *registers;
struct interrupts *interrupts;

/* Initialize DCPU16 internal state. Memory, registers, interrupts.
 * Open a shm file for modules to connect to.
 */
void sim_init(void)
{
	int shm_fd;

	if ((shm_fd = shm_open("/dcpu16s-mem-shm", O_CREAT | O_EXCL | O_RDWR,
			0644)) == -1) {
		perror("core: shm_open mem");
		exit(1);
	}
	if (ftruncate(shm_fd, MEM_SIZE) == -1) {
		perror("core: ftruncate mem");
		exit(1);
	}
	if ((memory = mmap(0, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
			shm_fd, 0)) == MAP_FAILED) {
		perror("core: mmap mem");
		exit(1);
	}

	if ((shm_fd = shm_open("/dcpu16s-reg-shm", O_CREAT | O_EXCL | O_RDWR,
			0644)) == -1) {
		perror("core: shm_open reg");
		exit(1);
	}
	if (ftruncate(shm_fd, REG_SIZE) == -1) {
		perror("core: ftruncate reg");
		exit(1);
	}
	if ((registers = mmap(0, REG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
			shm_fd, 0)) == MAP_FAILED) {
		perror("core: mmap reg");
		exit(1);
	}

	if ((shm_fd = shm_open("/dcpu16s-int-shm", O_CREAT | O_EXCL | O_RDWR,
			0644)) == -1) {
		perror("core: shm_open int");
		exit(1);
	}
	if (ftruncate(shm_fd, MEM_SIZE) == -1) {
		perror("core: ftruncate int");
		exit(1);
	}
	if ((interrupts = mmap(0, INT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
			shm_fd, 0)) == MAP_FAILED) {
		perror("core: mmap int");
		exit(1);
	}

	interrupts->enabled = 1;
}

/* Close the shm file so that no more modules can connect while running.
 */
void sim_start(void)
{
	if (shm_unlink("/dcpu16s-mem-shm") == -1) {
		perror("core: shm_unlink mem");
	}
	if (shm_unlink("/dcpu16s-reg-shm") == -1) {
		perror("core: shm_unlink reg");
	}
	if (shm_unlink("/dcpu16s-int-shm") == -1) {
		perror("core: shm_unlink int");
	}
}

/* Unmap shared memory, preparing to shutdown the simulator
 */
void sim_stop(void)
{
	if (munmap(memory, MEM_SIZE) == -1) {
		perror("core: munmap memory");
	}
	if (munmap(registers, REG_SIZE) == -1) {
		perror("core: munmap registers");
	}
	if (munmap(interrupts, INT_SIZE) == -1) {
		perror("core: munmap interrupts");
	}
}
