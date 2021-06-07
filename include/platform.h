#ifndef PLATFORM_H
#define PLATFORM_H

#include "machine.h"


/**
 * Runs without interruption
 */
void platform_run(Machine *machine);


/**
 * Steps through one instruction at a time
 */
void platform_step(Machine *machine);

#endif
