#ifndef _HARDWARE_HOST_H
#define _HARDWARE_HOST_H

#include <stdint.h>
#include <sys/types.h>

enum {
	HARDWARE_NONE = 0,
	HARDWARE_BUILTIN,
	HARDWARE_MODULE
};

struct hw_builtin {
	uint32_t id;
	uint16_t version;
	uint32_t mfg;

	void (*init)(void);
	int (*interrupt)(void);
	void (*step)(void);
};

struct hw_module {
	pid_t pid;
};

union hw {
	struct hw_module *module;
	struct hw_builtin *builtin;
};

struct hardware {
	int type;
	union hw hw;
};

void attach_hardware_builtin(void);
int attach_hardware_module(const char *name);
void hardware_deinit(void);
void hardware_hwi(uint16_t where);
void hardware_hwq(uint16_t where);
void hardware_step_all(void);
uint16_t hardware_get_attached(void);

#endif
