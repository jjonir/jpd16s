#include <stdio.h>
#include "core.h"

static int load_bin(const char *filename);

int main()
{
	load_bin("core.bin");
	run_dcpu16();
	return 0;
}

int load_bin(const char *filename)
{
	FILE *f;

	if ((f = fopen(filename, "r")) == NULL)
		perror("fopen");
}
