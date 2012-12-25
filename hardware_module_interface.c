#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "hardware.h"

static uint16_t get_uint16(void);
static void set_input_blocking(void);
static void set_input_nonblocking(void);

uint16_t get_uint16(void)
{
	char buf[32];
	uint16_t ret;

	set_input_blocking();
	fgets(buf, 32, stdin);
	set_input_nonblocking();
	sscanf(buf, "%hu", &ret);

	return ret;
}

void set_input_blocking(void)
{
	int fl = fcntl(0, F_GETFL);
	fcntl(0, F_SETFL, fl & ~O_NONBLOCK);
}

void set_input_nonblocking(void)
{
	int fl = fcntl(0, F_GETFL);
	fcntl(0, F_SETFL, fl | O_NONBLOCK);
}

uint16_t read_register(uint16_t reg)
{
	printf("reg r %hu\n", reg);
	fflush(stdout);
	return get_uint16();
}

void read_memory(uint16_t addr, uint16_t len, uint16_t *buf)
{
	int i;
	printf("mem r %hu %hu\n", addr, len);
	fflush(stdout);
	for (i = 0; i < len; i++)
		buf[i] = get_uint16();
}

void write_register(uint16_t reg, uint16_t val)
{
	printf("reg w %hu %hu\n", reg, val);
	fflush(stdout);
}

void write_memory(uint16_t addr, uint16_t len, uint16_t *buf)
{
	int i;
	printf("mem w %hu %hu", addr, len);
	fflush(stdout);
	for (i = 0; i < len; i++) {
		printf(" %hu", buf[i]);
		fflush(stdout);
	}
	printf("\n");
	fflush(stdout);
}

void raise_interrupt(uint16_t msg)
{
	printf("int %hu\n", msg);
	fflush(stdout);
}
