#ifndef MACHINE_H
#define MACHINE_H

#include <inttypes.h>
#include "cpu.h"


// ports 0-6
#define __PORT_COUNT 7

#define P2_START 1
#define P1_START 2
#define P1_FIRE 4
#define P1_JOY_LEFT 5
#define P1_JOY_RIGHT 6

#define P2_FIRE (P1_FIRE * 2)
#define P2_JOY_LEFT (P1_JOY_LEFT * 2)
#define P2_JOY_RIGHT (P1_JOY_RIGHT * 2)

typedef struct machine_t {
    uint16_t shift_register;
    State8080 *cpu_state; 
    IO8080 *io;
    uint8_t ports [__PORT_COUNT];
} Machine;


/**
 * Executes one CPU instruction
 * through the machine
 */
void machine_step(Machine* machine);

/**
 * Insert coin into machine
 */
void machine_insert_coin(Machine *machine);


/**
 * Handles key presses
 */
void machine_keydown(Machine *machine, char key);

/**
 * Handles key releases
 */
void machine_keyup(Machine *machine, char key);

#endif
