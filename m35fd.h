#ifndef _M35FD_H
#define _M35FD_H

#include "hardware.h"

enum m35fd_state {
	STATE_NO_MEDIA,
	STATE_READY,
	STATE_READY_WP,
	STATE_BUSY
};

enum m35fd_error {
	ERROR_NONE,
	ERROR_BUSY,
	ERROR_NO_MEDIA,
	ERROR_PROTECTED,
	ERROR_EJECT,
	ERROR_BAD_SECTOR,
	ERROR_BROKEN
};

struct hw_builtin *m35fd(uint16_t *buf);

#endif
