#ifndef CPU_H
#define CPU_H

#include <inttypes.h>
#include <stdint.h>

typedef struct condition_codes_t {
    // zero: set if result is 0
    uint8_t     z:1;  // z occupies 1 bit in the struct

    // sign: set if result is negative
    uint8_t     s:1;

    // parity: set if number of 1 bits in the
    // result is even
    uint8_t     p:1;

    // carry: set if last addition operation
    // resulted in a carry or if the last 
    // subtraction operation required a borrow
    uint8_t     cy:1; // not to be confused with C register

    // auxiliary carry: used for binary-coded
    // decimal arithmetic
    uint8_t     ac:1;

    // padding so that the struct occupies exactly
    // 8 bits
    uint8_t     pad:3;
} ConditionCodes;


/**
 * External I/O interface for 8080.
 * 
 * Whenever an IN or OUT instruction is executed,
 * the `in` or `out` flag is set to 1. If it's
 * an OUT instruction, the machine should set the
 * value of the port to `value`. If it's an IN
 * instruction, the machine should set the value
 * of its port to `value`.
 */
typedef struct io8080_t {
    // port number
    uint8_t port;

    // port value
    uint8_t value;

    // flag for IN (1 if active)
    uint8_t in:1;

    // flag for OUT (1 if active)
    uint8_t out:1;
} IO8080;


typedef struct state8080_t {
    // registers (7 of them)
    uint8_t             a;
    uint8_t             b;
    uint8_t             c;
    uint8_t             d;
    uint8_t             e;
    uint8_t             h;
    uint8_t             l;

    // stack pointer
    uint16_t            sp;

    // program counter
    uint16_t            pc;

    uint8_t             *memory;

    // status flags
    ConditionCodes      cc;

    // 1 if interrupt enabled
    uint8_t             int_enable;
    uint8_t             int_pending;
    uint8_t             int_delay;
    uint8_t             int_type;

    unsigned long       cycles;
} State8080;


/*
 * Prints out the current state
 */
void cpu_print_state(State8080 *state);


/**
 * Returns the current opcode
 */
uint8_t cpu_curr_op(State8080 *state);

/**
 * Returns true if the given IO struct
 * is empty (has default values)
 */
uint8_t cpu_io_empty(IO8080 io);


/**
 * Resets the IO object to default (empty) values
 */
void cpu_io_reset(IO8080 *io);


/*
 * Given the state, emulates the opcode
 * pointed to by the program counter
 * and moves onto the next instruction
 */
int cpu_emulate_op(State8080 *state, IO8080 *io);


/**
 * Generates an interrupt. `interrupt_num` is
 * the interrupt number (1 or 2) rather than the opcode
 * itself.
 */
void cpu_request_interrupt(State8080 *state, int interrupt_num);


/**
 * Returns the framebuffer from memory
 */
void* cpu_framebuffer(State8080 *state);

#endif
