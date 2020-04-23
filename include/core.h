#ifndef CORE_H
#define CORE_H

#include <inttypes.h>

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
} State8080;


void print_state(State8080 *state);

#endif
