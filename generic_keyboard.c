#include "core.h"
#include "hardware.h"
#include "hardware_host.h"

#define KBD_BUF_LEN 0x20 // TODO determine a good buffer length
static uint16_t kbd_buf[KBD_BUF_LEN];
static uint16_t read_ptr, write_ptr;
static int int_enabled = 0;
static uint16_t int_msg;

static void kbd_init(void);
static int kbd_interrupt(void);
static void kbd_step(void);

struct hw_builtin kbd_builtin = {
	0x30cf7406,
	0x0001,
	0x00000000,
	&kbd_init,
	&kbd_interrupt,
	&kbd_step
};

struct hw_builtin *generic_keyboard(void)
{
	struct hw_builtin *kbd;

	kbd = (struct hw_builtin *)malloc(sizeof(struct hw_builtin));
	*kbd = kbd_builtin;
	return kbd;
}

#ifdef BUILD_MODULE
struct hw_builtin *get_hw(void)
{
	return generic_keyboard();
}
#endif

void kbd_init(void)
{
	read_ptr = write_ptr = 0;
}

int kbd_interrupt(void)
{
	uint16_t b;

	switch(registers->A) {
	case 0:
		read_ptr = write_ptr = 0;
		break;
	case 1:
		if (read_ptr == write_ptr) {
			registers->C = 0;
		} else {
			registers->C = kbd_buf[read_ptr++];
			if (read_ptr == KBD_BUF_LEN)
				read_ptr = 0;
		}
		break;
	case 2:
		b = registers->B;
		if (pressed(b))
			registers->C = 1;
		else
			registers->C = 0;
		break;
	case 3:
		b = registers->B;
		if (b) {
			int_enabled = 1;
			int_msg = b;
		} else {
			int_enabled = 0;
		}
		break;
	default:
		break;
	}

	return 0;
}

void kbd_step(void)
{
}
