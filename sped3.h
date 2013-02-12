#ifndef _SPED3_H
#define _SPED3_H

enum sped3_state {
	SPED_STATE_NO_DATA = 0x0000,
	SPED_STATE_RUNNING = 0x0001,
	SPED_STATE_TURNING = 0x0002
};

enum sped3_error {
	SPED_ERROR_NONE = 0x0000,
	SPED_ERROR_BROKEN = 0xFFFF,
};

#ifndef BUILD_MODULE
struct hw_builtin *sped3(void);
#endif

#endif
