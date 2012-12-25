#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "hardware.h"
#include "hardware_host.h"

extern struct hw_builtin *get_hw(void);
static void hwq(struct hw_builtin * hw);

int main(int argc, char *argv[])
{
	struct hw_builtin *hw;
	char input[32];
	char *status;

	fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
	
	hw = get_hw();

	printf("%s\n", argv[0]);
	fflush(stdout);
	printf("ready\n");
	fflush(stdout);
	while (!feof(stdin)) {
		(*hw->step)();
		status = fgets(input, 32, stdin);
		if (NULL != status) {
			//fprintf(stderr, "got %s\n", input);
			if (strcmp(input, "hwi\n") == 0)
				(*hw->interrupt)();
			else if (strcmp(input, "hwq\n") == 0)
				hwq(hw);
			printf("done\n");
			fflush(stdout);
		}
	}

	return 0;
}

void hwq(struct hw_builtin *hw)
{
	write_register(REG_A, hw->id & 0xFFFF);
	write_register(REG_B, hw->id >> 16);
	write_register(REG_C, hw->version);
	write_register(REG_X, hw->mfg & 0xFFFF);
	write_register(REG_Y, hw->mfg >> 16);
}
