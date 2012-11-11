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

	if ((f = fopen(filename, "rb")) == NULL) {
		perror("fopen");
		return -1;
	}
	if (fread((void *)memory, 2, 0x10000, f) == 0) {
		fprintf(stderr, "core file unreadable or empty\n");
		return -2;
	}
	if (!feof(f)) {
		fprintf(stderr, "core file larger than the maximum 0x10000 words\n");
		return -3;
	}
	fclose(f);

	return 0;
}
