#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include "core.h"
#include "hardware.h"
#include "hardware_host.h"

static volatile sig_atomic_t sig1, sig2, sigint;
uint16_t *memory;
struct register_file *registers;
uint16_t *register_array;
volatile uint16_t *int_vec;
uint16_t hwid;
int int_sem;

extern struct hw_builtin *get_hw(void);
static void hwq(struct hw_builtin * hw);
static void usr_handler(int n);
static void int_handler(int n);

int main(int argc, char *argv[])
{
	struct hw_builtin *hw;
	struct sigaction sa_usr, sa_int;
	key_t key;
	int shmid;

	// TODO error handling
	key = ftok("jpd16s", 'm');
	shmid = shmget(key, 0x10000 * sizeof(uint16_t), 0644 | IPC_CREAT);
	memory = (uint16_t *)shmat(shmid, (void *)0, 0);

	key = ftok("jpd16s", 'r');
	shmid = shmget(key, 12 * sizeof(uint16_t), 0644 | IPC_CREAT);
	register_array = (uint16_t *)shmat(shmid, (void *)0, 0);
	registers = (struct register_file *)register_array;

	key = ftok("jpd16s", 'i');
	shmid = shmget(key, 0x10000 * sizeof(uint16_t), 0644 | IPC_CREAT);
	int_vec = (uint16_t *)shmat(shmid, (void *)0, 0);

	key = ftok("jpd16s", 'i');
	int_sem = semget(key, 1, 0644);

	hwid = registers->A;
	registers->A = 0;

	sigint = 0;
	sa_int.sa_handler = &int_handler;
	sigemptyset(&sa_int.sa_mask);
	sa_int.sa_flags = 0;
	sigaction(SIGINT, &sa_int, NULL);

	hw = get_hw();

	sig1 = sig2 = 0;
	sa_usr.sa_handler = &usr_handler;
	sigemptyset(&sa_usr.sa_mask);
	sa_usr.sa_flags = 0;
	sigaction(SIGUSR1, &sa_usr, NULL);
	sigaction(SIGUSR2, &sa_usr, NULL);

	while (sigint == 0) {
		(*hw->step)();
		if (sig1) {
			(*hw->interrupt)();
			sig1 = 0;
		}
		if (sig2) {
			hwq(hw);
			sig2 = 0;
		}
	}

	shmdt(memory);
	shmdt(registers);
	shmdt((void *)int_vec);

	return 0;
}

void hwq(struct hw_builtin *hw)
{
	registers->A = hw->id & 0xFFFF;
	registers->B = hw->id >> 16;
	registers->C = hw->version;
	registers->X = hw->mfg & 0xFFFF;
	registers->Y = hw->mfg >> 16;
}

void raise_interrupt(uint16_t msg)
{
	extern uint16_t hwid; // TODO
	struct sembuf sembuf;
	sembuf.sem_num = 0;
	sembuf.sem_op = -1;
	sembuf.sem_flg = 0;
	semop(int_sem, &sembuf, 1);
	int_vec[hwid] = msg;
	sembuf.sem_num = 0;
	sembuf.sem_op = 1;
	sembuf.sem_flg = 0;
	semop(int_sem, &sembuf, 1);
	//fprintf(stderr, "hwid %i raising interrupt msg %i\n", hwid, msg);
	//getchar();
}

void usr_handler(int n)
{
	if (n == SIGUSR1)
		sig1 = 1;
	else if (n == SIGUSR2)
		sig2 = 1;
}

void int_handler(int n)
{
	(void)n;
	sigint = 1;
}
