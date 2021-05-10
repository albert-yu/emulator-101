#include "machine.h"

// Port 1
//  bit 0 = CREDIT (1 if deposit)
//  bit 1 = 2P start (1 if pressed)
//  bit 2 = 1P start (1 if pressed)
//  bit 3 = Always 1
//  bit 4 = 1P shot (1 if pressed)
//  bit 5 = 1P left (1 if pressed)
//  bit 6 = 1P right (1 if pressed)
//  bit 7 = Not connected


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
uint8_t machine_in(Machine *machine, uint8_t port) {
    uint8_t a = 0;
    switch (port) {
        case 0:
            // not used
            break;
        case 3:
        {
            uint16_t v = machine->shift_register;
            a = (v >> (8 - machine->shift_offset)) & 0xff;
        }
            break;
    }
    return a;
}


/**
 * Handles data flow from CPU to machine
 */
void machine_out(Machine *machine, uint8_t port, uint8_t value) {
    switch (port) {
        case 2:
        {
            // right-most three bits (0x7 = 0b111)
            uint8_t shift_offset = value & 0x7;
            machine->shift_offset = shift_offset;
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


void machine_step(Machine *machine) {
    IO8080 *io = &machine->io;
    emulate_op(&machine->cpu_state, io);
    if (io_empty(*io)) {
        return;
    }

    if (io->in) {
        uint8_t a = machine_in(machine, io->port);
        io->value = a;
    } else if (io->out) {
        machine_out(machine, io->port, io->value);
    }
}
