#ifndef _HARDWARE_H
#define _HARDWARE_H

#include <stdint.h>

struct hardware {
	uint16_t id[2];
	uint16_t version;
	uint16_t mfg[2];

	void (*init)(void);
	uint16_t (*interrupt)(void);
	void (*step)(void);
};

#endif
