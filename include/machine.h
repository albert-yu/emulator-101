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

#define FRAME_ROWS 256
#define FRAME_COLS 224


// type alias for time stamp
typedef double timestamp;

typedef struct machine_t {
    // special hardware for shifts
    uint16_t shift_register;

    // CPU
    State8080 *cpu_state; 

    // I/O
    IO8080 *io;

    // machine's ports
    uint8_t ports [__PORT_COUNT];

    // time stamp of last time
    timestamp last_ts;

    // time stamp of next interrupt
    timestamp next_int;

    // type of interrupt (1 or 2)
    int int_type;

    // total cycles
    unsigned long cycles;
} Machine;


/**
 * Executes one CPU instruction
 * through the machine and returns
 * the number of cycles
 */
int machine_step(Machine* machine);

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


/**
 * Runs the machine and delays execution for the specified
 * number of micro seconds
 */
void machine_run(Machine *machine, long sleep_microseconds);


/**
 * Returns the frame buffer
 */
void* machine_framebuffer(Machine *machine);

#endif
