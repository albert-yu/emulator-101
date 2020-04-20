#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#include "core.h"

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

    uint8_t             int_enable;
} State8080;

/*
 * Print out the state for debugging
 */
void print_state(State8080 *state) {
    printf("\n");
    // in hex
    printf("Registers:\n");
    printf("A: %x\n", state->a);
    printf("B: %x\n", state->b);
    printf("C: %x\n", state->c);
    printf("D: %x\n", state->d);
    printf("E: %x\n", state->e);
    printf("H: %x\n", state->h);
    printf("L: %x\n", state->l);
    printf("\n");

    printf("Stack pointer: %x\n", state->sp);
    printf("Program counter: %x\n", state->pc);

    // TODO: print memory dump

    printf("Status flags:\n");
    printf("Z:  %d\n", state->cc.z);
    printf("S:  %d\n", state->cc.s);
    printf("P:  %d\n", state->cc.p);
    printf("CY: %d\n", state->cc.cy);
    printf("AC: %d\n", state->cc.ac);
    printf("\n");
    // TODO: figure out what this is
    printf("Int enable: %d\n", state->int_enable);
    printf("\n");
}

void unimplemented_instr(State8080 *state) {
    printf("Error: Unimplemented instruction\n");
    exit(1);
}

// Flags

uint8_t zero(uint16_t answer) {
    // set to 1 if answer is 0, 0 otherwise
    return ((answer & 0xff) == 0);
}

uint8_t sign(uint16_t answer) {
    // set to 1 when bit 7 of the math instruction is set
    return ((answer & 0x80) == 0);
}

uint8_t parity(uint16_t answer) {
    uint8_t ans8bit = answer & 0xff;
    // 1 (true) if even, 0 otherwise
    return ((ans8bit & 0x01) == 0); 
}

uint8_t carry(uint16_t answer) {
    // set to 1 when instruction resulted in a carry or borrow into the high order bit
    return (answer > 0xff); 
}

uint8_t auxcarry(uint16_t answer) {
    // skip implementation because Space Invaders doesn't use it
    // TODO: do this
    return 0;  
}


// combine with bitwise OR
// to set flags 
#define SET_Z_FLAG  1 << 7
#define SET_S_FLAG  1 << 6
#define SET_P_FLAG  1 << 5
#define SET_CY_FLAG 1 << 4
#define SET_AC_FLAG 1 << 3

/*
 * Set the specified flags according to the answer received by
 * arithmetic
 * flagstoset - from left to right, the z, s, p, cy, and ac flags (should
 * set flag if set to 1)
 */
void set_flags(State8080 *state, uint16_t answer, uint8_t flagstoset) {
    // remove trailing bits
    uint8_t cleaned = flagstoset & 0b11111000;
    if (cleaned & SET_Z_FLAG) {
        state->cc.z = zero(answer);
    }
    if (cleaned & SET_S_FLAG) {
        state->cc.s = sign(answer);
    }
    if (cleaned & SET_P_FLAG) {
        state->cc.p = parity(answer);
    }
    if (cleaned & SET_CY_FLAG) {
        state->cc.cy = carry(answer);
    }
    if (cleaned & SET_AC_FLAG) {
        state->cc.ac = auxcarry(answer); 
    }
}

void emulate_op(State8080 *state) {
    unsigned char *opcode = &state->memory[state->pc];

    switch(*opcode) {
        case 0x00:  // NOP
            break;

        case 0x01: 
        {
            state->c = opcode[1];  // c <- byte 2
            state->b = opcode[2];  // b <- byte 3
            state->pc += 2;  // advance two more bytes
        }
            break;

        case 0x02: unimplemented_instr(state); break;
        case 0x03: unimplemented_instr(state); break;
        case 0x04: 
        {
            uint16_t answer = (uint16_t) state->b + 1;
            // state->cc.z = zero(answer);
            // state->cc.s = sign(answer);
            // state->cc.p = parity(answer); // 1 (true) if even, 0 otherwise
            // state->cc.ac = auxcarry(answer); 
            uint8_t flags = SET_Z_FLAG | SET_S_FLAG | SET_P_FLAG | SET_AC_FLAG;
            set_flags(state, answer, flags);
            state->b = answer & 0xff;  // b <- b + 1
        }
            break;

        case 0x05: 
        {
            uint8_t answer = state->b - 1;
            uint8_t flags = SET_Z_FLAG | SET_S_FLAG | SET_P_FLAG | SET_AC_FLAG;
            set_flags(state, answer, flags);
            state->b = answer & 0xff;
        }           
            break;

        case 0x06: 
        {
            state->b = opcode[1];  // b <- byte 2
            state->pc += 1;
        }           
            break;

        case 0x07:  // A = A << 1; bit 0 = prev bit 7; CY = prev bit 7
        {
            // get left-most bit
            uint8_t leftmost = state->a >> 7;
            state->cc.cy = leftmost;
            // Set right-most bit to whatever the left-most bit is
            state->a = state->a << 1;
            state->a = state->a | leftmost;
        }
            break;

        case 0x08:  // NOP
            break;
        case 0x09: unimplemented_instr(state); break;
        case 0x0a: unimplemented_instr(state); break;
        case 0x0b: unimplemented_instr(state); break;
        case 0x0c: // INR c
        {
            uint16_t answer = ((uint16_t) state->c) + 1; 
            state->c = (uint8_t) answer;
            uint8_t flags = SET_Z_FLAG | SET_S_FLAG | SET_P_FLAG | SET_AC_FLAG;
            set_flags(state, answer, flags);
        }
            
            break;
        case 0x0d: unimplemented_instr(state); break;
        case 0x0e: unimplemented_instr(state); break;
        case 0x0f: unimplemented_instr(state); break;
        case 0x10: unimplemented_instr(state); break;
        case 0x11: unimplemented_instr(state); break;
        case 0x12: unimplemented_instr(state); break;
        case 0x13: unimplemented_instr(state); break;
        case 0x14: unimplemented_instr(state); break;
        case 0x15: unimplemented_instr(state); break;
        case 0x16: unimplemented_instr(state); break;
        case 0x17: unimplemented_instr(state); break;
        case 0x18: unimplemented_instr(state); break;
        case 0x19: unimplemented_instr(state); break;
        case 0x1a: unimplemented_instr(state); break;
        case 0x1b: unimplemented_instr(state); break;
        case 0x1c: unimplemented_instr(state); break;
        case 0x1d: unimplemented_instr(state); break;
        case 0x1e: unimplemented_instr(state); break;
        case 0x1f: unimplemented_instr(state); break;
        case 0x20: unimplemented_instr(state); break;
        case 0x21: unimplemented_instr(state); break;
        case 0x22: unimplemented_instr(state); break;
        case 0x23: unimplemented_instr(state); break;
        case 0x24: unimplemented_instr(state); break;
        case 0x25: unimplemented_instr(state); break;
        case 0x26: unimplemented_instr(state); break;
        case 0x27: unimplemented_instr(state); break;
        case 0x28: unimplemented_instr(state); break;
        case 0x29: unimplemented_instr(state); break;
        case 0x2a: unimplemented_instr(state); break;
        case 0x2b: unimplemented_instr(state); break;
        case 0x2c: unimplemented_instr(state); break;
        case 0x2d: unimplemented_instr(state); break;
        case 0x2e: unimplemented_instr(state); break;
        case 0x2f: unimplemented_instr(state); break;
        case 0x30: unimplemented_instr(state); break;
        case 0x31: unimplemented_instr(state); break;
        case 0x32: unimplemented_instr(state); break;
        case 0x33: unimplemented_instr(state); break;
        case 0x34: unimplemented_instr(state); break;
        case 0x35: unimplemented_instr(state); break;
        case 0x36: unimplemented_instr(state); break;
        case 0x37: unimplemented_instr(state); break;
        case 0x38: unimplemented_instr(state); break;
        case 0x39: unimplemented_instr(state); break;
        case 0x3a: unimplemented_instr(state); break;
        case 0x3b: unimplemented_instr(state); break;
        case 0x3c: unimplemented_instr(state); break;
        case 0x3d: unimplemented_instr(state); break;
        case 0x3e: unimplemented_instr(state); break;
        case 0x3f: unimplemented_instr(state); break;
        case 0x40: unimplemented_instr(state); break;
        case 0x41:  // MOV B,C
            state->b = state->c; 
            break;
        case 0x42:  // MOV B,D
            state->b = state->d; 
            break;
        case 0x43:  // MOV B,E
            state->b = state->e; 
            break;
        case 0x44:  // etc.
            state->b = state->h;
            break;
        case 0x45: 
            state->b = state->l;
            break;
        case 0x46: 
        // 8-bit H and 8-bit L registers can be used as 
        // one 16-bit HL register pair. When used as a 
        // pair the L register contains low-order byte. 
        // HL register usually contains a data pointer 
        // used to reference memory addresses.
            // state->b = state->m;
            unimplemented_instr(state);
            break;
        case 0x47: 
            state->b = state->a;
            break;
        case 0x48: unimplemented_instr(state); break;
        case 0x49: unimplemented_instr(state); break;
        case 0x4a: unimplemented_instr(state); break;
        case 0x4b: unimplemented_instr(state); break;
        case 0x4c: unimplemented_instr(state); break;
        case 0x4d: unimplemented_instr(state); break;
        case 0x4e: unimplemented_instr(state); break;
        case 0x4f: unimplemented_instr(state); break;
        case 0x50: unimplemented_instr(state); break;
        case 0x51: unimplemented_instr(state); break;
        case 0x52: unimplemented_instr(state); break;
        case 0x53: unimplemented_instr(state); break;
        case 0x54: unimplemented_instr(state); break;
        case 0x55: unimplemented_instr(state); break;
        case 0x56: unimplemented_instr(state); break;
        case 0x57: unimplemented_instr(state); break;
        case 0x58: unimplemented_instr(state); break;
        case 0x59: unimplemented_instr(state); break;
        case 0x5a: unimplemented_instr(state); break;
        case 0x5b: unimplemented_instr(state); break;
        case 0x5c: unimplemented_instr(state); break;
        case 0x5d: unimplemented_instr(state); break;
        case 0x5e: unimplemented_instr(state); break;
        case 0x5f: unimplemented_instr(state); break;
        case 0x60: unimplemented_instr(state); break;
        case 0x61: unimplemented_instr(state); break;
        case 0x62: unimplemented_instr(state); break;
        case 0x63: unimplemented_instr(state); break;
        case 0x64: unimplemented_instr(state); break;
        case 0x65: unimplemented_instr(state); break;
        case 0x66: unimplemented_instr(state); break;
        case 0x67: unimplemented_instr(state); break;
        case 0x68: unimplemented_instr(state); break;
        case 0x69: unimplemented_instr(state); break;
        case 0x6a: unimplemented_instr(state); break;
        case 0x6b: unimplemented_instr(state); break;
        case 0x6c: unimplemented_instr(state); break;
        case 0x6d: unimplemented_instr(state); break;
        case 0x6e: unimplemented_instr(state); break;
        case 0x6f: unimplemented_instr(state); break;
        case 0x70: unimplemented_instr(state); break;
        case 0x71: unimplemented_instr(state); break;
        case 0x72: unimplemented_instr(state); break;
        case 0x73: unimplemented_instr(state); break;
        case 0x74: unimplemented_instr(state); break;
        case 0x75: unimplemented_instr(state); break;
        case 0x76: unimplemented_instr(state); break;
        case 0x77: unimplemented_instr(state); break;
        case 0x78: unimplemented_instr(state); break;
        case 0x79: unimplemented_instr(state); break;
        case 0x7a: unimplemented_instr(state); break;
        case 0x7b: unimplemented_instr(state); break;
        case 0x7c: unimplemented_instr(state); break;
        case 0x7d: unimplemented_instr(state); break;
        case 0x7e: unimplemented_instr(state); break;
        case 0x7f: unimplemented_instr(state); break;
        case 0x80:  // ADD B
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->b;    
            state->cc.z = zero(answer);    
            state->cc.s = sign(answer);    
            state->cc.cy = carry(answer);    
            state->cc.p = parity(answer); 
            state->a = answer & 0xff;
        }            
            break;

        case 0x81:  // ADD C
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->c;    
            state->cc.z = zero(answer);    
            state->cc.s = sign(answer);    
            state->cc.cy = carry(answer);    
            state->cc.p = parity(answer);    
            state->a = answer & 0xff;
        }
            break;

        case 0x82:
        {
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->d;    
            state->cc.z = zero(answer);    
            state->cc.s = sign(answer);    
            state->cc.cy = carry(answer);    
            state->cc.p = parity(answer & 0xff);    
            state->a = answer & 0xff;
        } 
            break;

        case 0x83: unimplemented_instr(state); break;
        case 0x84: unimplemented_instr(state); break;
        case 0x85: unimplemented_instr(state); break;
        case 0x86: unimplemented_instr(state); break;
        case 0x87: unimplemented_instr(state); break;
        case 0x88: unimplemented_instr(state); break;
        case 0x89: unimplemented_instr(state); break;
        case 0x8a: unimplemented_instr(state); break;
        case 0x8b: unimplemented_instr(state); break;
        case 0x8c: unimplemented_instr(state); break;
        case 0x8d: unimplemented_instr(state); break;
        case 0x8e: unimplemented_instr(state); break;
        case 0x8f: unimplemented_instr(state); break;
        case 0x90: unimplemented_instr(state); break;
        case 0x91: unimplemented_instr(state); break;
        case 0x92: unimplemented_instr(state); break;
        case 0x93: unimplemented_instr(state); break;
        case 0x94: unimplemented_instr(state); break;
        case 0x95: unimplemented_instr(state); break;
        case 0x96: unimplemented_instr(state); break;
        case 0x97: unimplemented_instr(state); break;
        case 0x98: unimplemented_instr(state); break;
        case 0x99: unimplemented_instr(state); break;
        case 0x9a: unimplemented_instr(state); break;
        case 0x9b: unimplemented_instr(state); break;
        case 0x9c: unimplemented_instr(state); break;
        case 0x9d: unimplemented_instr(state); break;
        case 0x9e: unimplemented_instr(state); break;
        case 0x9f: unimplemented_instr(state); break;
        case 0xa0: unimplemented_instr(state); break;
        case 0xa1: unimplemented_instr(state); break;
        case 0xa2: unimplemented_instr(state); break;
        case 0xa3: unimplemented_instr(state); break;
        case 0xa4: unimplemented_instr(state); break;
        case 0xa5: unimplemented_instr(state); break;
        case 0xa6: unimplemented_instr(state); break;
        case 0xa7: unimplemented_instr(state); break;
        case 0xa8: unimplemented_instr(state); break;
        case 0xa9: unimplemented_instr(state); break;
        case 0xaa: unimplemented_instr(state); break;
        case 0xab: unimplemented_instr(state); break;
        case 0xac: unimplemented_instr(state); break;
        case 0xad: unimplemented_instr(state); break;
        case 0xae: unimplemented_instr(state); break;
        case 0xaf: unimplemented_instr(state); break;
        case 0xb0: unimplemented_instr(state); break;
        case 0xb1: unimplemented_instr(state); break;
        case 0xb2: unimplemented_instr(state); break;
        case 0xb3: unimplemented_instr(state); break;
        case 0xb4: unimplemented_instr(state); break;
        case 0xb5: unimplemented_instr(state); break;
        case 0xb6: unimplemented_instr(state); break;
        case 0xb7: unimplemented_instr(state); break;
        case 0xb8: unimplemented_instr(state); break;
        case 0xb9: unimplemented_instr(state); break;
        case 0xba: unimplemented_instr(state); break;
        case 0xbb: unimplemented_instr(state); break;
        case 0xbc: unimplemented_instr(state); break;
        case 0xbd: unimplemented_instr(state); break;
        case 0xbe: unimplemented_instr(state); break;
        case 0xbf: unimplemented_instr(state); break;
        case 0xc0: unimplemented_instr(state); break;
        case 0xc1: unimplemented_instr(state); break;
        case 0xc2: 
            if (state->cc.z == 0)
            {
                state->pc = (opcode[2] << 8 | opcode[1]);
            }
            else
            {
                // branch not taken
                state->pc += 2;
            }
            break;
        case 0xc3: 
            state->pc = (opcode[2] << 8 | opcode[1]);
            break;
        case 0xc4: unimplemented_instr(state); break;
        case 0xc5: unimplemented_instr(state); break;
        case 0xc6: unimplemented_instr(state); break;
        case 0xc7: unimplemented_instr(state); break;
        case 0xc8: unimplemented_instr(state); break;
        case 0xc9: unimplemented_instr(state); break;
        case 0xca: unimplemented_instr(state); break;
        case 0xcb: unimplemented_instr(state); break;
        case 0xcc: unimplemented_instr(state); break;
        case 0xcd: unimplemented_instr(state); break;
        case 0xce: unimplemented_instr(state); break;
        case 0xcf: unimplemented_instr(state); break;
        case 0xd0: unimplemented_instr(state); break;
        case 0xd1: unimplemented_instr(state); break;
        case 0xd2: unimplemented_instr(state); break;
        case 0xd3: unimplemented_instr(state); break;
        case 0xd4: unimplemented_instr(state); break;
        case 0xd5: unimplemented_instr(state); break;
        case 0xd6: unimplemented_instr(state); break;
        case 0xd7: unimplemented_instr(state); break;
        case 0xd8: unimplemented_instr(state); break;
        case 0xd9: unimplemented_instr(state); break;
        case 0xda: unimplemented_instr(state); break;
        case 0xdb: unimplemented_instr(state); break;
        case 0xdc: unimplemented_instr(state); break;
        case 0xdd: unimplemented_instr(state); break;
        case 0xde: unimplemented_instr(state); break;
        case 0xdf: unimplemented_instr(state); break;
        case 0xe0: unimplemented_instr(state); break;
        case 0xe1: unimplemented_instr(state); break;
        case 0xe2: unimplemented_instr(state); break;
        case 0xe3: unimplemented_instr(state); break;
        case 0xe4: unimplemented_instr(state); break;
        case 0xe5: unimplemented_instr(state); break;
        case 0xe6: unimplemented_instr(state); break;
        case 0xe7: unimplemented_instr(state); break;
        case 0xe8: unimplemented_instr(state); break;
        case 0xe9: unimplemented_instr(state); break;
        case 0xea: unimplemented_instr(state); break;
        case 0xeb: unimplemented_instr(state); break;
        case 0xec: unimplemented_instr(state); break;
        case 0xed: unimplemented_instr(state); break;
        case 0xee: unimplemented_instr(state); break;
        case 0xef: unimplemented_instr(state); break;
        case 0xf0: unimplemented_instr(state); break;
        case 0xf1: unimplemented_instr(state); break;
        case 0xf2: unimplemented_instr(state); break;
        case 0xf3: unimplemented_instr(state); break;
        case 0xf4: unimplemented_instr(state); break;
        case 0xf5: unimplemented_instr(state); break;
        case 0xf6: unimplemented_instr(state); break;
        case 0xf7: unimplemented_instr(state); break;
        case 0xf8: unimplemented_instr(state); break;
        case 0xf9: unimplemented_instr(state); break;
        case 0xfa: unimplemented_instr(state); break;
        case 0xfb: unimplemented_instr(state); break;
        case 0xfc: unimplemented_instr(state); break;
        case 0xfd: unimplemented_instr(state); break;
        case 0xfe: unimplemented_instr(state); break;
        case 0xff: unimplemented_instr(state); break;
    }

    state->pc += 1;
}

