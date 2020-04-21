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

uint8_t sign32(uint32_t answer) {
    // set to 1 when bit 15 of the math instruction is set
    return ((answer & 0x8000) == 0);
}

/*
 * Returns 0 if number of bits is even and 1 o.w.
 */
uint8_t parity(uint16_t answer) {
    uint8_t parity = 0;
    uint8_t ans8bit = answer & 0xff;
    while (ans8bit) {
        parity = !parity;
        ans8bit = ans8bit & (ans8bit - 1);
    }
    return parity;
}

uint8_t carry(uint16_t answer) {
    // set to 1 when instruction resulted in a carry or borrow into the high order bit
    return (answer > 0xff); 
}

uint8_t carry32(uint32_t answer) {
    return (answer > 0xffff);
}

uint8_t auxcarry(uint16_t answer) {
    // skip implementation because Space Invaders doesn't use it
    // TODO: do this
    return 0;  
}

uint8_t auxcarry32(uint32_t answer) {
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
#define SET_ALL_FLAGS (SET_Z_FLAG | SET_S_FLAG | SET_P_FLAG | SET_CY_FLAG | SET_AC_FLAG)

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

/*
 * Same as set_flags, except for a 32-bit answer
 * (adding/subtracting two 16-bit ints)
 */
void set_flags32(State8080 *state, uint32_t answer, uint8_t flagstoset) {
    // remove trailing bits
    uint8_t cleaned = flagstoset & 0b11111000;

    // split into left and right
    uint16_t left, right;
    left = answer >> 16;
    right = answer & 0xffff;
    if (cleaned & SET_Z_FLAG) {
        // should be 0 if both left and
        // right are 0
        state->cc.z = (zero(left) | zero(right)) == 0;
    }
    if (cleaned & SET_S_FLAG) {
        state->cc.s = sign32(answer);
    }
    if (cleaned & SET_P_FLAG) {
        // both odd => combined even
        // both even => combined even
        // even and odd => combined odd
        state->cc.p = (parity(left) ^ parity(right)) == 0;
    }
    if (cleaned & SET_CY_FLAG) {
        state->cc.cy = carry32(answer);
    }
    if (cleaned & SET_AC_FLAG) {
        state->cc.ac = auxcarry32(answer); 
  }
}

/*
 * Performs an add and stores the result in A
 * ADD X: A <- A + X
 * (instructions 0x80 to 0x87)
 */
void add_x(State8080 *state, uint8_t x) {
    uint16_t a = (uint16_t) state->a;
    uint16_t answer = a + (uint16_t) x;
    set_flags(state, answer, SET_ALL_FLAGS);
    state->a = answer & 0xff;
}

/*
 * Performs an add carry
 * ADC X: A <- A + X + CY
 */
void adc_x(State8080 *state, uint8_t x) {
    uint16_t a = (uint16_t) state->a;
    uint8_t cy = state->cc.cy;
    uint16_t answer = a + cy + x;
    set_flags(state, answer, SET_ALL_FLAGS);
    state->a = answer & 0xff;
}

/*
 * Performs a sub and stores the result in A
 * SUB X: A <- A - X
 */
void sub_x(State8080 *state, uint8_t x) {
    uint16_t a = (uint16_t) state->a;
    uint16_t answer = a - (uint16_t) x;
    set_flags(state, answer, SET_ALL_FLAGS);
    state->a = answer & 0xff;
}

/*
 * Performs a sub carry
 * SBB X: A <- A - X - CY
 */
void sbb_x(State8080 *state, uint8_t x) {
    uint16_t a = (uint16_t) state->a;
    uint8_t cy = state->cc.cy;
    uint16_t answer = a - x - cy;
    set_flags(state, answer, SET_ALL_FLAGS);
    state->a = answer & 0xff;
}

/*
 * Bitwise AND
 * ANA X: A <- A & A
 */
void ana_x(State8080 *state, uint8_t x) {
    // using 16 bits, even though
    // bitwise AND shouldn't add a bit
    uint16_t answer;
    answer = (uint16_t) state->a & x;
    set_flags(state, answer, SET_ALL_FLAGS);
    state->a = answer & 0xff;
}

/*
 * Bitwise XOR
 * XRA X: A <- A ^ X
 */
void xra_x(State8080 *state, uint8_t x) {
    uint16_t answer = (uint16_t) state->a ^ x;
    set_flags(state, answer, SET_ALL_FLAGS);
    state->a = answer & 0xff;
}

/*
 * Bitwise OR
 * ORA X: A <- A | X
 */
void ora_x(State8080 *state, uint8_t x) {
    uint16_t answer = (uint16_t) state->a | x;
    set_flags(state, answer, SET_ALL_FLAGS);
    state->a = answer & 0xff;
}

/*
 * Combines two 8 bit values into a single
 * 16 bit value
 */
uint16_t get16bitval(uint8_t left, uint8_t right) {
    uint16_t result;
    result = (left << 8) | right;
    return result;
}

/**
 * Adds the `val` to the 16-bit number stored by `left_ptr`
 * and `right_ptr` collectively and stores it back in
 * the two pointers. Also returns the result as 32 bits.
 */
uint32_t tworeg_add(uint8_t *left_ptr, uint8_t *right_ptr, uint16_t val) {
    uint8_t left, right;
    left = *left_ptr;
    right = *right_ptr;
    uint16_t summand;
    summand = get16bitval(left, right);
    uint32_t result = summand + val;
    *left_ptr = result >> 8;
    *right_ptr = result & 0xff;
    return result;
}

/*
 * Emulates INR (decrement register) instruction
 * INR X: X <- X + 1
 */
void inr_x(State8080 *state, uint8_t *ptr) {
    uint16_t answer = (uint16_t) *ptr + 1;
    uint8_t flags = SET_Z_FLAG | SET_S_FLAG | SET_P_FLAG | SET_AC_FLAG;
    set_flags(state, answer, flags);
    *ptr = answer & 0xff;
}

/*
 * Emulates DCR (increment register) instruction
 * INR X: X <- X + 1
 */
void dcr_x(State8080 *state, uint8_t *ptr) {
    uint16_t answer = (uint16_t) *ptr - 1;
    uint8_t flags = SET_Z_FLAG | SET_S_FLAG | SET_P_FLAG | SET_AC_FLAG;
    set_flags(state, answer, flags);
    *ptr = answer & 0xff;
}

/*
 * INX XY: XY <- XY + 1
 */
void inx_xy(uint8_t *left_ptr, uint8_t *right_ptr) {
    tworeg_add(left_ptr, right_ptr, 1);
    // INX does not set the carry bit
}

/*
 * DCX XY: XY <- XY + 1
 */
void dcx_xy(uint8_t *left_ptr, uint8_t *right_ptr) {
    tworeg_add(left_ptr, right_ptr, -1);
    // DCX does not set the carry bit
}

/*
 * DAD XY: HL <- HL + XY
 * and sets CY flag to 1 if result needs carry
 */
void dad_xy(State8080 *state, uint8_t *x, uint8_t *y) {
    uint8_t *dest_left, *dest_right; 
    dest_left = &(state->h);
    dest_right = &(state->l);
    uint16_t val_to_add;
    val_to_add = get16bitval(*x, *y);
    uint32_t result = tworeg_add(dest_left, dest_right, val_to_add);
    state->cc.cy = result > 0xffff;
}

/*
 * Returns the 16 bit address pointed to by
 * two pointers to 8 bit integers
 */
uint16_t read_addr(uint8_t *lptr, uint8_t *rptr) {
   return get16bitval(*lptr, *rptr);
}

/*
 * Returns the address stored in HL register
 * pair
 */
uint16_t read_hl_addr(State8080 *state) {
    return read_addr(&(state->h), &(state->l));
}

/*
 * Reads the value in memory pointed to by
 * the HL register pair
 */
uint8_t read_hl(State8080 *state) {
    // Note: the addend is the byte pointed to by the address stored
    // in the HL register pair

    // get the address
    uint16_t offset = read_hl_addr(state);

    // get value in memory
    uint8_t m = state->memory[offset];
    return m;
}

void emulate_op(State8080 *state) {
    unsigned char *opcode = &state->memory[state->pc];

    switch(*opcode) {
        case 0x00:  // NOP
            break;

        case 0x01:  // LXI B,D16
        {
            state->c = opcode[1];  // c <- byte 2
            state->b = opcode[2];  // b <- byte 3
            state->pc += 2;  // advance two more bytes
        }
            break;

        case 0x02:  // STAX B: (BC) <- A
        {
            // set the value of memory with address formed by
            // register pair BC to A
            uint16_t offset = read_addr(&state->b, &state->c);
            state->memory[offset] = state->a;
        }
            break;
        case 0x03:   // INX B
        {
            // BC <- BC + 1 
            inx_xy(&state->b, &state->c);
        }
            break;
        case 0x04: 
        {
            inr_x(state, &state->b);
        }
            break;

        case 0x05:
        {
            dcr_x(state, &state->b); 
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
            // set right-most bit to whatever the left-most bit is
            state->a = state->a << 1;
            state->a = state->a | leftmost;
        }
            break;

        case 0x08:  // NOP
            break;
        case 0x09:  // DAD B: HL = HL + BC
        {
            dad_xy(state, &state->b, &state->c);
        }
            break;
        case 0x0a:  // LDAX B: A <- (BC)
        {
            uint16_t offset = read_addr(&state->b, &state->c);
            uint8_t memval = state->memory[offset];
            state->a = memval;
        }
            break;
        case 0x0b:  // DCX B: BC <- BC - 1
        {
            dcx_xy(&state->b, &state->c);
        }
            break;
        case 0x0c:  // INR C
        {
            inr_x(state, &state->c);
        }
            
            break;
        case 0x0d:  // DCR C
        {
            dcr_x(state, &state->c);
        }
            break;
        case 0x0e: unimplemented_instr(state); break;
        case 0x0f: unimplemented_instr(state); break;
        case 0x10: unimplemented_instr(state); break;
        case 0x11: unimplemented_instr(state); break;
        case 0x12: unimplemented_instr(state); break;
        case 0x13:
        {
            // pointers to registers
            inx_xy(&state->d, &state->e);
        }
            break;
        case 0x14:  // INR D
        {
            inr_x(state, &state->d);
        }
            break;
        case 0x15:
        {
            dcr_x(state, &state->d);
        }
            break;
        case 0x16: unimplemented_instr(state); break;
        case 0x17: unimplemented_instr(state); break;
        case 0x18: unimplemented_instr(state); break;
        case 0x19:  // DAD D: HL = HL + DE
        {
            uint8_t *d_reg_ptr, *e_reg_ptr;
            d_reg_ptr = &state->d;
            e_reg_ptr = &state->e;
            dad_xy(state, d_reg_ptr, e_reg_ptr);
            // uint32_t answer;
            // uint16_t hl, de;
            // hl = get16bitval(state->h, state->l);
            // de = get16bitval(state->d, state->e);
            // answer = hl + de;
            // state->cc.cy = answer > 0xffff;

            // // store answer in H and L
            // state->h = answer >> 8;
            // state->l = answer & 0xff;
        }
            break;
        case 0x1a:  // LDAX D
        {
            uint16_t offset = read_addr(&state->d, &state->e);
            uint8_t memval = state->memory[offset];
            state->a = memval;
        }
            break;
        case 0x1b:
        {
            dcx_xy(&state->d, &state->e);
        }
            break;
        case 0x1c:
        {
            inr_x(state, &state->e);
        }
            break;
        case 0x1d:
        {
            dcr_x(state, &state->e);
        }
            break;
        case 0x1e: unimplemented_instr(state); break;
        case 0x1f: unimplemented_instr(state); break;
        case 0x20: unimplemented_instr(state); break;
        case 0x21: unimplemented_instr(state); break;
        case 0x22: unimplemented_instr(state); break;
        case 0x23:
        {
            uint8_t *h_ptr, *l_ptr;
            h_ptr = &state->h;
            l_ptr = &state->l;
            inx_xy(h_ptr, l_ptr);
        }
            break;
        case 0x24:  // INR H
        {
            inr_x(state, &state->h);
        }
            break;
        case 0x25:
        {
            dcr_x(state, &state->h);
        }
            break;
        case 0x26: unimplemented_instr(state); break;
        case 0x27: unimplemented_instr(state); break;
        case 0x28: unimplemented_instr(state); break;
        case 0x29:  // DAD H
        {
            uint8_t *h_reg_ptr, *l_reg_ptr;
            h_reg_ptr = &state->h;
            l_reg_ptr = &state->l;
            dad_xy(state, h_reg_ptr, l_reg_ptr);
        }
            break;
        case 0x2a: unimplemented_instr(state); break;
        case 0x2b:  // DCX H: HL <- HL - 1
        {
            dcx_xy(&state->h, &state->l);
        }
            break;
        case 0x2c:  // INR L
        {
            inr_x(state, &state->l);
        }
            break;
        case 0x2d:
        {
            dcr_x(state, &state->l);
        }
            break;
        case 0x2e: unimplemented_instr(state); break;
        case 0x2f: unimplemented_instr(state); break;
        case 0x30: unimplemented_instr(state); break;
        case 0x31: unimplemented_instr(state); break;
        case 0x32: unimplemented_instr(state); break;
        case 0x33:  // INX SP: SP <- SP + 1
        {
            // stack pointer is already 16 bits
            state->sp = state->sp + 1; 
        }
            break;
        case 0x34:
        {
            // need to get the pointer
            // to update memory in correct place
            uint16_t offset = read_hl_addr(state);
            uint8_t *m_ptr = &state->memory[offset];
            inr_x(state, m_ptr);
        }
            break;
        case 0x35:
        {
            uint16_t offset = read_hl_addr(state);
            uint8_t *m_ptr = &state->memory[offset];
            dcr_x(state, m_ptr);
        }
            break;
        case 0x36: unimplemented_instr(state); break;
        case 0x37: unimplemented_instr(state); break;
        case 0x38: unimplemented_instr(state); break;
        case 0x39:  // DAD SP
        {
            // uglier implementation
            uint32_t answer;
            uint8_t *h_reg_ptr, *l_reg_ptr;
            h_reg_ptr = &state->h;
            l_reg_ptr = &state->l;
            answer = tworeg_add(h_reg_ptr, l_reg_ptr, state->sp);
            state->cc.cy = answer > 0xffff;
        }
            break;
        case 0x3a: unimplemented_instr(state); break;
        case 0x3b:  // DCX SP
        {
            uint16_t curr_sp = state->sp;
            state->sp = curr_sp - 1;
            // no flags set
        }
            break;
        case 0x3c:  // INR A
        {
            inr_x(state, &state->a);
        }
            break;
        case 0x3d:
        {
            dcr_x(state, &state->a);
        }
            break;
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
        case 0x7f:  // MOV A,A
            break;
        case 0x80:  // ADD B
        {
            add_x(state, state->b);
        }            
            break;

        case 0x81:  // ADD C
        {
            add_x(state, state->c);
        }
            break;

        case 0x82:  // ADD D
        {
            add_x(state, state->d);
        } 
            break;

        case 0x83:  // ADD E
        {
            add_x(state, state->e);
        } 
            break;
        case 0x84:  // ADD H
        {
            add_x(state, state->h);
        } 
            break;
        case 0x85:  // ADD L
        {
            add_x(state, state->l);
        }
            break;
        case 0x86:  // ADD M
        {
            uint8_t m = read_hl(state);
            add_x(state, m);
        }
            break;
        case 0x87:  // ADD A
        {
            add_x(state, state->a);
        }
            break;
        case 0x88:  // ADC B (A <- A + B + CY)
        {
            uint8_t b = state->b;
            adc_x(state, b);
        }
            break;
        case 0x89:  // ADC C
        {
            adc_x(state, state->c);
        }
            break;
        case 0x8a:  // ADC D
        {
            adc_x(state, state->d);
        }
            break;
        case 0x8b:  // ADC E
        {
            adc_x(state, state->e);
        }
            break;
        case 0x8c:  // ADC H 
        {
            adc_x(state, state->h);
        }
            break;
        case 0x8d:  // ADC L
        {
            adc_x(state, state->l);
        }
            break;
        case 0x8e:
        {
            uint8_t m = read_hl(state);
            adc_x(state, m);
        }
        case 0x8f: 
        {
            adc_x(state, state->a);
        }
            break;
        case 0x90:  // SUB B
        {
            sub_x(state, state->b);
        }
            break;
        case 0x91:
        {
            sub_x(state, state->c);
        }
            break;
        case 0x92:
        {
            sub_x(state, state->d);
        }
            break;
        case 0x93:
        {
            sub_x(state, state->e);
        }
            break;
        case 0x94:
        {
            sub_x(state, state->h);
        }
            break;
        case 0x95:
        {
            sub_x(state, state->l);
        }
            break;
        case 0x96:  // SUB (HL)
        {
            uint8_t m = read_hl(state);
            sub_x(state, m);
        }
            break;
        case 0x97:  // SUB A
        {
            sub_x(state, state->a);
        }
            break;
        case 0x98:  // SBB B
        {
            sbb_x(state, state->b);
        }
            break;
        case 0x99:
        {
            sbb_x(state, state->c);
        }
            break;
        case 0x9a:
        {
            sbb_x(state, state->d);
        }
            break;
        case 0x9b:
        {
            sbb_x(state, state->e);
        }
            break;
        case 0x9c:
        {
            sbb_x(state, state->h);
        }
            break;
        case 0x9d:
        {
            sbb_x(state, state->l);
        }
            break;
        case 0x9e:
        {
            uint8_t m = read_hl(state);
            sbb_x(state, m);
        }
            break;
        case 0x9f:
        {
            sbb_x(state, state->a);
        }
            break;
        case 0xa0:  // ANA B
        {
            ana_x(state, state->b);
        }
            break;
        case 0xa1:
        {
            ana_x(state, state->c);
        }
            break;
        case 0xa2:
        {
            ana_x(state, state->d);
        }
            break;
        case 0xa3:
        {
            ana_x(state, state->e);
        }
            break;
        case 0xa4:
        {
            ana_x(state, state->h);
        }
            break;
        case 0xa5:
        {
            ana_x(state, state->l);
        }
            break;
        case 0xa6:
        {
            uint8_t m = read_hl(state);
            ana_x(state, m);
        }
            break;
        case 0xa7:
        {
            ana_x(state, state->a);
        }
            break;
        case 0xa8:
        {
            xra_x(state, state->b);
        }
            break;
        case 0xa9:
        {
            xra_x(state, state->c);
        }
            break;
        case 0xaa:
        {
            xra_x(state, state->d);
        }
            break;
        case 0xab:
        {
            xra_x(state, state->e);
        }
            break;
        case 0xac:
        {
            xra_x(state, state->h);
        }
            break;
        case 0xad:
        {
            xra_x(state, state->l);
        }
            break;
        case 0xae:
        {
            uint8_t m = read_hl(state);
            xra_x(state, m);
        }
            break;
        case 0xaf:
        {
            xra_x(state, state->a);
        }
            break;
        case 0xb0:
        {
            ora_x(state, state->b);
        }
            break;
        case 0xb1:
        {
            ora_x(state, state->c);
        }
            break;
        case 0xb2:
        {
            ora_x(state, state->d);
        }
            break;
        case 0xb3:
        {
            ora_x(state, state->e);
        }
            break;
        case 0xb4:
        {
            ora_x(state, state->h);
        }
            break;
        case 0xb5:
        {
            ora_x(state, state->l);
        }
            break;
        case 0xb6:
        {
            uint8_t m = read_hl(state);
            ora_x(state, m);
        }
            break;
        case 0xb7:
        {
            ora_x(state, state->a);
        }
            break;
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
        case 0xc6: 
        {
            // The immediate form is the almost the 
            // same except the source of the addend 
            // is the byte after the instruction. 
            // Since "opcode" is a pointer to the 
            // current instruction in memory, 
            // opcode[1] will be the immediately following byte.
            uint16_t answer = (uint16_t) state->a + (uint16_t) opcode[1];
            set_flags(state, answer, SET_ALL_FLAGS);

            // instruction is of size 2
            state->pc += 1;
        }
            break;
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
        case 0xd6:   // SUI D8
        {
            uint8_t data = opcode[1];
            uint16_t answer = (uint16_t) state->a - (uint16_t) data;
            set_flags(state, answer, SET_ALL_FLAGS);
            state->a = answer & 0xff;
            
            state->pc += 1;
        } 
            break;
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

