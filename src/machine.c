#include <time.h>
#include "machine.h"

#define EPSILON 1e-9

// 16 bit shift register:

// 	f              0	bit
// 	xxxxxxxxyyyyyyyy
	
// 	Writing to port 4 shifts x into y, and the new value into x, eg.
// 	$0000,
// 	write $aa -> $aa00,
// 	write $ff -> $ffaa,
// 	write $12 -> $12ff, ..
	
// 	Writing to port 2 (bits 0,1,2) sets the offset for the 8 bit result, eg.
// 	offset 0:
// 	rrrrrrrr		result=xxxxxxxx
// 	xxxxxxxxyyyyyyyy
	
// 	offset 2:
// 	  rrrrrrrr	result=xxxxxxyy
// 	xxxxxxxxyyyyyyyy
	
// 	offset 7:
// 	       rrrrrrrr	result=xyyyyyyy
// 	xxxxxxxxyyyyyyyy
	
// 	Reading from port 3 returns said result.

/**
 * Handles data flow from machine to CPU
 */
uint8_t machine_in_cpu(Machine *machine, uint8_t port) {
    uint8_t a = 0;
    switch (port) {
        case 0:
            return 1;
        case 1:
            return 2;
        case 3:
        {
            uint16_t v = machine->shift_register;
            uint8_t shift_offset = machine->ports[2];
            a = (v >> (8 - shift_offset)) & 0xff;
            // machine->ports.in3 = a; // useless assignment
        }
            break;
    }
    return a;
}


/**
 * Handles data flow from CPU to machine
 */
void machine_out_cpu(Machine *machine, uint8_t port, uint8_t value) {
    switch (port) {
        case 2:
        {
            // right-most three bits (0x7 = 0b111)
            uint8_t shift_offset = value & 0x7;
            machine->ports[2] = shift_offset;
        }
            break;
        case 4:
        {
            uint16_t curr_val = machine->shift_register;

            // shift the right 8 bits to the right
            // by 8, and put value as left 8 bits
            uint16_t new_val = (value << 8) | (curr_val >> 8);
            machine->shift_register = new_val;
        }
            break;
    }
}


/**
 * Get UNIX timestamp in microseconds
 * as a float
 */
timestamp ts_utc_micro() {
    struct timespec time;
    // gettimeofday(&time, NULL);
    timespec_get(&time, TIME_UTC);
    return ((double)time.tv_sec * 1e6) + ((double) time.tv_nsec) / 1000.0;
}


#define FPS (1.0 / 60.0)
#define INTERVAL_MICROSEC (FPS * 1e6)


void machine_step(Machine *machine) {
    IO8080 *io = machine->io;
    emulate_op(machine->cpu_state, io);
    if (io_empty(*io)) {
        return;
    }

    if (io->in) {
        uint8_t a = machine_in_cpu(machine, io->port);
        io->value = a;
    } else if (io->out) {
        machine_out_cpu(machine, io->port, io->value);
    }

    timestamp now = ts_utc_micro();
    if (machine->last_ts < EPSILON) {
        machine->last_ts = now;
        machine->next_int = machine->last_ts + INTERVAL_MICROSEC;
        machine->int_type = 1;
    }

    if (machine->cpu_state->int_enable && now > machine->next_int) {
        interrupt(machine->cpu_state, machine->int_type);

        // trick to flip between 1 and 2
        machine->int_type = 3 - machine->int_type;
        machine->next_int = now + INTERVAL_MICROSEC / 2.0;
    }
}

#define P2_START_BIT_SET (1 << P2_START)
#define P1_START_BIT_SET (1 << P1_START)

// fire is bit 4 for both player 1 and 2
#define FIRE_BIT_SET (1 << P1_FIRE)

// joy left is bit 5 for both player 1 and 2
#define LEFT_BIT_SET (1 << P1_JOY_LEFT)

// joy right is bit 6 for both player 1 and 2
#define RIGHT_BIT_SET (1 << P1_JOY_RIGHT)

#define P2_START_BIT_UNSET ~P2_START_BIT_SET
#define P1_START_BIT_UNSET ~P1_START_BIT_SET
#define FIRE_BIT_UNSET ~FIRE_BIT_SET
#define LEFT_BIT_UNSET ~LEFT_BIT_SET
#define RIGHT_BIT_UNSET ~RIGHT_BIT_SET


void machine_keydown(Machine *machine, char key) {
    switch (key) {
        case P2_START:
            machine->ports[1] |= P2_START_BIT_SET;
            break;
        case P1_START:
            machine->ports[1] |= P1_START_BIT_SET;
            break;
        case P1_FIRE:
            machine->ports[1] |= FIRE_BIT_SET;
            break;
        case P1_JOY_LEFT: 
            // set bit 5 of port 1
            machine->ports[1] |= LEFT_BIT_SET;
            break;
        case P1_JOY_RIGHT:
            // set bit 6 of port 1
            machine->ports[1] |= RIGHT_BIT_SET;
            break;
        case P2_FIRE:
            machine->ports[2] |= FIRE_BIT_SET;
            break;
        case P2_JOY_LEFT: 
            // set bit 5 of port 1
            machine->ports[2] |= LEFT_BIT_SET;
            break;
        case P2_JOY_RIGHT:
            // set bit 6 of port 1
            machine->ports[2] |= RIGHT_BIT_SET;
            break;
    }
}


void machine_keyup(Machine *machine, char key) {
    switch (key) {
        case P2_START:
            machine->ports[1] &= P2_START_BIT_UNSET;
            break;
        case P1_START:
            machine->ports[1] &= P1_START_BIT_UNSET;
            break;
        case P1_FIRE:
            machine->ports[1] &= FIRE_BIT_UNSET;
            break;
        case P1_JOY_LEFT: 
            machine->ports[1] &= LEFT_BIT_UNSET;
            break;
        case P1_JOY_RIGHT:
            machine->ports[1] &= RIGHT_BIT_UNSET;
            break;
        case P2_FIRE:
            machine->ports[2] &= FIRE_BIT_UNSET;
            break;
        case P2_JOY_LEFT: 
            // set bit 5 of port 1
            machine->ports[2] &= LEFT_BIT_UNSET;
            break;
        case P2_JOY_RIGHT:
            // set bit 6 of port 1
            machine->ports[2] &= RIGHT_BIT_UNSET;
            break;
    }
}


void machine_insert_coin(Machine *machine) {
    machine->ports[1] |= 1;
}
