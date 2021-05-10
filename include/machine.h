#ifndef MACHINE_H
#define MACHINE_H

#include <inttypes.h>
#include "cpu.h"

typedef struct machine_t {
    uint16_t shift_register;
    uint8_t shift_offset;
    State8080 cpu_state; 
    IO8080 io;
} Machine;


/**
 * Executes one CPU instruction
 * through the machine
 */
void machine_step(Machine* machine);

#endif
