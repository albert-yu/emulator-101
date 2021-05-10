#ifndef MACHINE_H
#define MACHINE_H

#include <inttypes.h>
#include "cpu.h"


// ports 0-6
#define PORT_COUNT 7

typedef struct machine_t {
    uint16_t shift_register;
    State8080 *cpu_state; 
    IO8080 *io;
    uint8_t ports [PORT_COUNT];
} Machine;


/**
 * Executes one CPU instruction
 * through the machine
 */
void machine_step(Machine* machine);

#endif
