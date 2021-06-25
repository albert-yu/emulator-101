#include <stdint.h>
#include <time.h>
#include <errno.h>
#include "cpu.h"
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
            a = machine->ports[0];
            break;
        case 1:
            a = machine->ports[1];
            break;
        case 2:
            a = machine->ports[2];
            break;
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
        case 3:
            // play sound
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
        case 5:
            // play sound
            break;
    }
}


/**
 * Get UNIX timestamp in microseconds
 * as a float
 */
timestamp ts_utc_micro() {
    struct timespec time;
    timespec_get(&time, TIME_UTC);
    return ((double)time.tv_sec * 1e6) + ((double) time.tv_nsec) / 1000.0;
}


#define MHZ 2
#define FPS 60


// real clock timing
#define INV_FPS (1.0 / FPS)
#define INTERVAL_MICROSEC (INV_FPS * 1e6 / MHZ)

// cycle timing
#define CYCLES_PER_FRAME (MHZ * 1e6 / FPS)

// number of cycles between interrupts (every half frame)
#define CYCLES_INTERVAL (CYCLES_PER_FRAME / 2)


void process_interrupts(Machine *machine) {
    if (machine->cycles < CYCLES_INTERVAL) {
        return;
    }

    if (machine->cpu_state->int_enable) {
        cpu_request_interrupt(machine->cpu_state, machine->int_type);
    }

    // trick to flip between 1 and 2
    machine->int_type = 3 - machine->int_type;
    machine->cycles -= CYCLES_INTERVAL;
}


int machine_step(Machine *machine) {
    IO8080 *io = machine->io;
    uint8_t opcode = cpu_curr_op(machine->cpu_state);
    int cycles = cpu_emulate_op(machine->cpu_state, io);
    machine->cycles += cycles;

    process_interrupts(machine);

    switch (opcode) {
        case 0xdb: // IN
            machine->cpu_state->a = machine_in_cpu(machine, io->port);
            break;
        case 0xd3: // OUT
            machine_out_cpu(machine, io->port, io->value);
            break;
    }

    return cycles;
}


/**
 * Time-aware machine execution
 * (synchronized at 2 MHz)
 */
void machine_do_sync(Machine *machine) {
    timestamp now = ts_utc_micro();
    if (machine->last_ts < EPSILON) {
        machine->last_ts = now;
        machine->next_int = machine->last_ts + INTERVAL_MICROSEC;
        machine->int_type = 1;
    }

    timestamp since_last = now - machine->last_ts;

    // 2 MHz clock speed, so 2 cycles per microsecond
    int cycles_to_catch_up = MHZ * since_last;

    int cycles = 0;
    while (cycles_to_catch_up > cycles) {
        cycles += machine_step(machine);
    }

    machine->last_ts = now;
}


int sleep_msec(long microseconds) {
    long nsec = (microseconds % 1000000) * 1000;
    long seconds = microseconds / 1e6;
    struct timespec ts = (struct timespec) {
        .tv_sec = seconds,
        .tv_nsec = nsec
    };
    int res;
    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);
    return res;
}


void machine_run(Machine *machine, long sleep_microseconds) {
    machine_do_sync(machine);
    sleep_msec(sleep_microseconds);
}


void* machine_framebuffer(Machine *machine) {
    return cpu_framebuffer(machine->cpu_state);
}


// Bits 1-3 always set
#define PORT_0_DEFAULT 0b00001110

// Bit 3 always set
#define PORT_1_DEFAULT 0b00001000


void machine_init_ports(Machine *machine) {
    machine->ports[0] = PORT_0_DEFAULT;
    machine->ports[1] = PORT_1_DEFAULT;
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


/**
 * Returns which port the key
 * will get mapped to
 */
uint8_t which_port(char key) {
    uint8_t port;
    switch (key) {
        case P2_START:
        case P1_START:
        case P1_FIRE:
        case P1_JOY_LEFT: 
        case P1_JOY_RIGHT:
        case INSERT_COIN:
            port = 1;
            break;
        case P2_FIRE:
        case P2_JOY_LEFT: 
        case P2_JOY_RIGHT:
            port = 2;
    } 
    return port;
}


void machine_insert_coin(Machine *machine) {
    machine->ports[1] |= 1;
}


void update_io(Machine *machine, char key) {
    uint8_t port = which_port(key);
    machine->io->value = machine->ports[port];
    machine->io->in = 1;
}


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
            // set bit 5 of port 2
            machine->ports[2] |= LEFT_BIT_SET;
            break;
        case P2_JOY_RIGHT:
            // set bit 6 of port 2
            machine->ports[2] |= RIGHT_BIT_SET;
            break;
        case INSERT_COIN:
            machine_insert_coin(machine);
            break;
    }

    update_io(machine, key);
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
        case INSERT_COIN:
            machine->ports[1] &= 0b11111110;
            break;
    }
    update_io(machine, key);
}
