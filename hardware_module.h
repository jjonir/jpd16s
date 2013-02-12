#ifndef _HARDWARE_MODULE_H
#define _HARDWARE_MODULE_H

#include <stdint.h>
#include <sys/types.h>

extern sig_atomic_t int_requested, hwq_requested;

int init_module(void);
int shutdown_module(void);
void hwq(uint32_t id, uint16_t ver, uint32_t mfg);

#endif
