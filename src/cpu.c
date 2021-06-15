#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "cpu.h"
#include "disassembler.h"

/**
 * CPU cycle lookup table
 */
uint8_t cycles[] = {
    4, 10, 7, 5, 5, 5, 7, 4, 4, 10, 7, 5, 5, 5, 7, 4, //0x00..0x0f
    4, 10, 7, 5, 5, 5, 7, 4, 4, 10, 7, 5, 5, 5, 7, 4, //0x10..0x1f
    4, 10, 16, 5, 5, 5, 7, 4, 4, 10, 16, 5, 5, 5, 7, 4, //etc
    4, 10, 13, 5, 10, 10, 10, 4, 4, 10, 13, 5, 5, 5, 7, 4,

    5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5, //0x40..0x4f
    5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
    5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
    7, 7, 7, 7, 7, 7, 7, 7, 5, 5, 5, 5, 5, 5, 7, 5,

    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4, //0x80..8x4f
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
    4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,

    11, 10, 10, 10, 17, 11, 7, 11, 11, 10, 10, 10, 10, 17, 7, 11, //0xc0..0xcf
    11, 10, 10, 10, 17, 11, 7, 11, 11, 10, 10, 10, 10, 17, 7, 11, 
    11, 10, 10, 18, 17, 11, 7, 11, 11, 5, 10, 5, 17, 17, 7, 11, 
    11, 10, 10, 4, 17, 11, 7, 11, 11, 5, 10, 4, 17, 17, 7, 11, 
};


#define ROM_START 0
#define ROM_END 0x1fff
#define RAM_START (ROM_END + 1)
#define RAM_END 0x23ff
#define FRAMEBUFFER_START 0x2400


void* cpu_framebuffer(State8080 *state) {
    return (void*) &state->memory[FRAMEBUFFER_START];
}


#define INSTRS_TO_PRINT 10

void print_instructions(State8080 *state) {
    uint16_t pc = state->pc;
    for (int i = 0; i < INSTRS_TO_PRINT; i++) {
        int steps_to_advance = disassemble8080op(state->memory, pc);
        pc += steps_to_advance;
    }
}


/*
 * Print out the state for debugging
 */
void cpu_print_state(State8080 *state) {
    printf("\n");
    printf("----------------------------------\n");
    printf(" A    B    C    D    E    H    L  \n");
    printf("0x%02x ", state->a);
    printf("0x%02x ", state->b);
    printf("0x%02x ", state->c);
    printf("0x%02x ", state->d);
    printf("0x%02x ", state->e);
    printf("0x%02x ", state->h);
    printf("0x%02x ", state->l);
    printf("\n");
    printf("----------------------------------\n");
    printf(" Z S P CY AC \n");
    printf(" %d", state->cc.z);
    printf(" %d", state->cc.s);
    printf(" %d", state->cc.p);
    printf(" %d", state->cc.cy);
    printf("  %d", state->cc.ac);
    printf("\n\n");
    printf(" SP: 0x%04x\n", state->sp);
    printf(" PC: 0x%04x\n", state->pc);
    printf(" Interrupt enable: %d\n", state->int_enable);
    printf("----------------------------------\n");
    printf("\n");
    print_instructions(state);
}


uint8_t cpu_io_empty(IO8080 io) {
    uint8_t any_filled = io.in || io.out || io.port || io.value;
    return !any_filled;
}


void print_failed_state(State8080 *state) {
    printf("State at failure:\n");
    cpu_print_state(state);
}


void unimplemented_instr(State8080 *state) {
    uint8_t opcode = state->memory[state->pc];
    printf("Error: Unimplemented instruction 0x%x\n", opcode);
    print_failed_state(state);
    exit(EXIT_FAILURE);
}


void unused_opcode(State8080 *state, uint8_t opcode) {
    printf("Error: unused opcode 0x%x\n", opcode);
    print_failed_state(state);
    exit(EXIT_FAILURE);
}


/**
 * Writes to memory only if the offset is valid.
 * Otherwise, exits the program with failure.
 */
void mem_write(State8080 *state, uint16_t offset, uint8_t value) {
    if (offset >= ROM_START && offset <= ROM_END) {
        printf("Fatal error: tried to write to ROM at address 0x%x\n", offset);
        print_failed_state(state);
        exit(EXIT_FAILURE);
    }
    state->memory[offset] = value;
}


/*
 * Combines two 8 bit values into a single
 * 16 bit value
 */
uint16_t makeword(uint8_t left, uint8_t right) {
    uint16_t result;
    result = (left << 8) | right;
    return result;
}


/**
 * Reads the byte at the specified location
 */
uint8_t mem_read(State8080 *state, uint16_t offset) {
    return state->memory[offset];
}


/**
 * Returns a pointer to the value at the specified
 * location in memory
 */
uint8_t* mem_read_ptr(State8080 *state, uint8_t offset) {
    return &state->memory[offset];
}


/**
 * Returns next byte pointed to by the program counter
 * and increments the program counter
 */
uint8_t next_byte(State8080 *state) {
    return mem_read(state, state->pc++);
}


/**
 * Returns next word pointed to by the program counter
 * and updates the program counter
 */
uint16_t next_word(State8080 *state) {
    uint8_t left, right;
    right = next_byte(state);
    left = next_byte(state);
    return makeword(left, right);
}


// Flags ----------------------------------

uint8_t zero(uint16_t answer) {
    // set to 1 if answer is 0, 0 otherwise
    return ((answer & 0xff) == 0);
}


uint8_t sign(uint16_t answer) {
    // set to 1 when bit 7 of the math instruction is set
    // return ((answer & 0x80) == 0x80);
    return (answer & 0xff) >> 7;
}


/*
 * Returns 1 if number of set bits is even and 0 o.w.
 * (only for lower 8 bits)
 */
uint8_t parity(uint16_t answer) {
    // uint8_t p = 0;
    // uint8_t ans8bit = answer & 0xff;
    // while (ans8bit) {
    //     p = !p;
    //     ans8bit = ans8bit & (ans8bit - 1);
    // }
    // return !p;
    //
    uint8_t x = answer & 0xff;
    int size = 8;
    int i;
    int p = 0;
    x = (x & ((1 << size) - 1));
    for (i = 0; i < size; i++) {
        if (x & 0x1) {
            p++;
        }
        x = x >> 1;
    }
    return (0 == (p & 0x1));
}


/*
 * Returns 1 when instruction resulted 
 * in a carry or borrow into the high order bit
 */
uint8_t carry(uint16_t answer) {
    return (answer > 0xff); 
}


uint8_t auxcarry(uint16_t answer) {
    // From the manual:
    // If the instruction caused a
    // carry out of bit 3 and into
    // bit 4 of the resulting value,
    // the auxiliary carry is set;
    // otherwise it is reset. This
    // flag is affected by single
    // precision additions,
    // subtractions, increments,
    // decrements, comparisons, and
    // logical operations, but is
    // principally used with
    // additions and increments
    // preceding a DAA (Decimal
    // Adjust Accumulator)
    // instruction.
    uint8_t last8, cleaned;
    last8 = answer & 0xff;
    // zero out first three bits
    //                  76543210
    cleaned = last8 & 0b00011111; 
    return cleaned > 0xf;  
}


// combine with bitwise OR
// to set flags 
#define SET_Z_FLAG  (1 << 7)
#define SET_S_FLAG  (1 << 6)
#define SET_P_FLAG  (1 << 5)
#define SET_CY_FLAG (1 << 4)
#define SET_AC_FLAG (1 << 3)
#define SET_ALL_FLAGS (SET_Z_FLAG | SET_S_FLAG | SET_P_FLAG | SET_CY_FLAG | SET_AC_FLAG)


/*
 * Set the specified flags according to the answer received by
 * arithmetic
 * flagstoset - from left to right, the z, s, p, cy, 
 * and ac flags (should set flag if set to 1)
 */
void set_arith_flags(State8080 *state, uint16_t answer, uint8_t flagstoset) {
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
 * Sets flags from a logic operation response
 */
void set_logic_flags(State8080 *state, uint8_t res, uint8_t flagstoset) {
    // remove trailing bits
    uint8_t cleaned = flagstoset & 0b11111000;
    uint16_t answer = (uint16_t) res;
    if (cleaned & SET_Z_FLAG) {
        state->cc.z = zero(answer);
    }
    if (cleaned & SET_S_FLAG) {
        state->cc.s = sign(answer);
    }
    if (cleaned & SET_P_FLAG) {
        state->cc.p = parity(answer);
    }

    // carry and aux carry flags are zero
    if (cleaned & SET_CY_FLAG) {
        state->cc.cy = 0;
    }
    if (cleaned & SET_AC_FLAG) {
        state->cc.ac = 0; 
  }
}


/*
 * JMP to address specified
 * in bytes 2 and 3
 */
void jmp(State8080 *state, uint16_t adr) {
    state->pc = adr;
}


/*
 * If cond, JMP adr
 */
void jmp_cond(State8080 *state, uint16_t adr, uint8_t cond) {
    if (cond) {
        jmp(state, adr);
    } else {
        // branch not taken
        state->pc += 2;
    }
}


/*
 * Pushes contents onto the stack.
 */
void push_x(State8080 *state, uint8_t hi, uint8_t lo) {
    uint16_t sp_addr = state->sp;
    mem_write(state, sp_addr - 1, hi);
    mem_write(state, sp_addr - 2, lo);
    state->sp -= 2;
}


/*
 * Call specified target address (need for RST)
 */
void call_adr(State8080 *state, uint16_t adr) {
    // get return address
    // to pick up where left
    // off
    uint16_t sp_addr, ret_addr;
    sp_addr = state->sp;
    ret_addr = state->pc + 2;

    // split return address
    // into two parts
    uint8_t hi_addr, lo_addr;
    hi_addr = (ret_addr >> 8) & 0xff;
    lo_addr = ret_addr & 0xff;

    // push return address onto the stack
    push_x(state, hi_addr, lo_addr);

    // set program counter to
    // target address
    state->pc = adr;
}


/*
 * CALL conditionally
 * If cond is TRUE, then CALL subr
 */
void call_cond(State8080 *state, uint16_t subr, uint8_t cond) {
    if (cond) {
        call_adr(state, subr);
    } else {
        // otherwise, move onto
        // next instruction
        state->pc += 2;
    }
}


/*
 * RET instruction
 */
void ret(State8080 *state) {
    uint16_t sp_addr = state->sp;
    uint8_t hi_addr, lo_addr;
    lo_addr = mem_read(state, sp_addr);
    hi_addr = mem_read(state, sp_addr + 1);
    // set pc to return address pointed
    // to by stack
    uint16_t target_addr = makeword(hi_addr, lo_addr);
    state->pc = target_addr;
    
    // increment stack pointer
    state->sp += 2;
}


/*
 * Pops content off the stack into 
 * registers `hi` and `lo`.
 */
void pop(State8080 *state, uint8_t *hi, uint8_t *lo) {
    uint16_t sp_addr;
    sp_addr = state->sp;
    *lo = mem_read(state, sp_addr);
    *hi = mem_read(state, sp_addr + 1);

    // increment stack pointer
    state->sp += 2;
}


/*
 * Performs an add and stores the result in A
 * ADD X: A <- A + X
 * (instructions 0x80 to 0x87)
 */
void add_x(State8080 *state, uint8_t x) {
    uint16_t a = (uint16_t) state->a;
    uint16_t answer = a + (uint16_t) x;
    set_arith_flags(state, answer, SET_ALL_FLAGS);
    state->a = answer & 0xff;
}


/*
 * Performs an add carry
 * ADC X: A <- A + X + CY
 */
void adc_x(State8080 *state, uint8_t x) {
    uint16_t a, cy, x16, answer;
    a = (uint16_t) state->a;
    cy = (uint16_t) state->cc.cy;
    x16 = (uint16_t) x;
    answer = a + cy + x16;
    set_arith_flags(state, answer, SET_ALL_FLAGS);
    state->a = answer & 0xff;
}


/*
 * Performs a sub and stores the result in A
 * SUB X: A <- A - X
 */
void sub_x(State8080 *state, uint8_t x) {
    uint16_t a = (uint16_t) state->a;
    uint16_t answer = a - (uint16_t) x;
    set_arith_flags(state, answer, 
        SET_ALL_FLAGS ^ SET_CY_FLAG);
    if (x > state->a) {
        state->cc.cy = 1;
    }
    state->a = answer & 0xff;
}


/*
 * Performs a sub carry
 * SBB X: A <- A - X - CY
 */
void sbb_x(State8080 *state, uint8_t x) {
    uint16_t a, cy, x16, answer;
    a = (uint16_t) state->a;
    cy = (uint16_t) state->cc.cy;
    x16 = (uint16_t) x;
    answer = a - x16 - cy;
    set_arith_flags(state, answer, 
        SET_ALL_FLAGS ^ SET_CY_FLAG);
    if (x16 + cy > a) {
        state->cc.cy = 1;
    }
    state->a = answer & 0xff;
}


/*
 * Bitwise AND
 * ANA X: A <- A & X
 */
void ana_x(State8080 *state, uint8_t x) {
    // using 16 bits, even though
    // bitwise AND shouldn't add a bit
    uint8_t answer;
    answer = state->a & x;
    set_logic_flags(state, answer, SET_ALL_FLAGS);
    state->a = answer & 0xff;
}


/*
 * Bitwise XOR
 * XRA X: A <- A ^ X
 */
void xra_x(State8080 *state, uint8_t x) {
    uint8_t answer = state->a ^ x;
    set_logic_flags(state, answer, SET_ALL_FLAGS);
    state->a = answer & 0xff;
}


/*
 * Bitwise OR
 * ORA X: A <- A | X
 */
void ora_x(State8080 *state, uint8_t x) {
    uint8_t answer = state->a | x;
    set_logic_flags(state, answer, SET_ALL_FLAGS);
    state->a = answer & 0xff;
}


/*
 * Swaps p1 with p2
 */
void swp_ptrs(uint8_t *p1, uint8_t *p2) {
    uint8_t tmp;
    tmp = *p1;
    *p1 = *p2;
    *p2 = tmp;
}


/*
 * Compare register
 * (A) - (r)
 * The content of register or memory location  (x) 
 * is subtracted from accumulator.
 * The accumulator remains unchanged. All flags are set.
 * Z flag is set to 1 if (A) = (r). CY set to 1 if (A) < (r).
 */
void cmp_x(State8080 *state, uint8_t x) {
    uint16_t answer;
    answer = (uint16_t) state->a - (uint16_t) x;
    set_arith_flags(state, answer, SET_ALL_FLAGS ^ SET_CY_FLAG);
    state->cc.cy = state->a < x;
}


/*
 * Adds the `val` to the 16-bit number stored by `left_ptr`
 * and `right_ptr` collectively and stores it back in
 * the two pointers. Also returns the result as 32 bits.
 */
uint32_t tworeg_add(uint8_t *left_ptr, uint8_t *right_ptr, uint16_t val) {
    // get values pointed to by pointers
    uint8_t left, right;
    left = *left_ptr;
    right = *right_ptr;

    // combine into summand
    uint16_t summand;
    summand = makeword(left, right);

    // sum
    uint32_t result = summand + val;

    // store
    *left_ptr = (result & 0xff00) >> 8;
    *right_ptr = result & 0xff;
    return result;
}


/*
 * Emulates INR (increment register) instruction
 * INR X: X <- X + 1
 */
void inr_x(State8080 *state, uint8_t *ptr) {
    uint16_t answer = (uint16_t) *ptr + 1;
    uint8_t flags = SET_Z_FLAG | SET_S_FLAG | SET_P_FLAG | SET_AC_FLAG;
    set_arith_flags(state, answer, flags);
    *ptr = answer & 0xff;
}


/*
 * Emulates DCR (decrement register) instruction
 * DCR X: X <- X - 1
 */
void dcr_x(State8080 *state, uint8_t *ptr) {
    uint16_t answer = (uint16_t) *ptr - 1;
    uint8_t flags = SET_Z_FLAG | SET_S_FLAG | SET_P_FLAG | SET_AC_FLAG;
    set_arith_flags(state, answer, flags);
    *ptr = answer & 0xff;
}


/*
 * INX XY: XY <- XY + 1
 */
void inx_xy(uint8_t *left_ptr, uint8_t *right_ptr) {
    //tworeg_add(left_ptr, right_ptr, 1);
    // INX does not set the carry bit
    (*right_ptr)++;
    if (*right_ptr == 0) {
        (*left_ptr)++;
    }
}


/*
 * DCX XY: XY <- XY - 1
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
    uint16_t val_to_add;
    val_to_add = makeword(*x, *y);
    uint32_t result = tworeg_add(
        &state->h, &state->l, val_to_add);
    state->cc.cy = ((result & 0xffff0000) != 0);
}


uint16_t bc_addr(State8080 *state) {
    return makeword(state->b, state->c);
}


/**
 * Reads the value in memory pointed to
 * by the BC register pair
 */
uint8_t get_bc_mem(State8080 *state) {
    return mem_read(state, bc_addr(state));
}


void set_bc_addr(State8080 *state, uint16_t addr) {
    uint8_t left = (addr >> 8) & 0xff;
    uint8_t right = addr & 0xff;
    state->b = left;
    state->c = right;
}


void set_bc_mem(State8080 *state, uint8_t val) {
    mem_write(state, bc_addr(state), val);
}


uint16_t de_addr(State8080 *state) {
    return makeword(state->d, state->e);
}


/**
 * Reads the value in memory pointed to
 * by the BC register pair
 */
uint8_t get_de_mem(State8080 *state) {
    return mem_read(state, de_addr(state));
}


void set_de_mem(State8080 *state, uint8_t val) {
    mem_write(state, de_addr(state), val);
}



/*
 * Returns the address stored in HL register
 * pair
 */
uint16_t hl_addr(State8080 *state) {
    return makeword(state->h, state->l);
}


/*
 * Reads the value in memory pointed to by
 * the HL register pair
 */
uint8_t get_hl_mem(State8080 *state) {
    return mem_read(state, hl_addr(state));
}


/**
 * Returns pointer to value in memory
 * pointed to by HL register pair
 */
uint8_t* get_hl_ptr(State8080 *state) {
    return mem_read_ptr(state, hl_addr(state));
}


/*
 * Sets the memory addressed by HL to `val`
 */
void set_hl_mem(State8080 *state, uint8_t val) {
    uint16_t offset = hl_addr(state);
    mem_write(state, offset, val);
}


void cpu_io_reset(IO8080 *io) {
    io->in = 0;
    io->out = 0;
    io->port = 0;
    io->value = 0;
}


void cpu_interrupt(State8080 *state, int interrupt_num) {
    uint8_t hi, lo;
    lo = state->pc & 0xff;
    hi = (state->pc >> 8) & 0xff;
    push_x(state, hi, lo);

    state->pc = 8 * interrupt_num;
}


int cpu_emulate_op(State8080 *state, IO8080 *io) {
    // read any in values
    if (io->in && io->value) {
        state->a = io->value;
    }

    cpu_io_reset(io);

    uint8_t *opcode = &state->memory[state->pc];
    state->pc += 1;

    switch (*opcode) {
        case 0x00:  // NOP
            break;
        case 0x01:  // LXI B,D16
        {
            // state->c = opcode[1];  // c <- byte 2
            // state->b = opcode[2];  // b <- byte 3
            // state->pc += 2;  // advance two more bytes
            set_bc_addr(state, next_word(state));
        }
            break;
        case 0x02:  // STAX B: (BC) <- A
        {
            // set the value of memory with address formed by
            // register pair BC to A
            uint16_t offset = makeword(state->b, state->c);
            mem_write(state, offset, state->a);
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
            // b <- byte 2
            state->b = next_byte(state);
            break;

        case 0x07:  // RLC: A = A << 1; bit 0 = prev bit 7; CY = prev bit 7
        {
            // get left-most bit
            uint8_t leftmost = state->a >> 7;
            state->cc.cy = leftmost;
            // set right-most bit to whatever the left-most bit was
            state->a = (state->a << 1) | leftmost;
        }
            break;

        case 0x08:
            unused_opcode(state, *opcode);
            break;
        case 0x09:  // DAD B: HL = HL + BC
        {
            dad_xy(state, &state->b, &state->c);
        }
            break;
        case 0x0a:  // LDAX B: A <- (BC)
            state->a = get_bc_mem(state);
            break;
        case 0x0b:  // DCX B: BC <- BC - 1
            dcx_xy(&state->b, &state->c);
            break;
        case 0x0c:  // INR C
            inr_x(state, &state->c);
            break;
        case 0x0d:  // DCR C
            dcr_x(state, &state->c);
            break;
        case 0x0e:  // MVI C,D8: C <- byte 2
            state->c = next_byte(state);
            break;
        case 0x0f:  // RRC: A = A >> 1; bit 7 = prev bit 0; CY = prev bit 0
        {
            // rotating bits right
            // e.g. 10011000 => 01001100
            uint8_t rightmost = state->a & 1;
            // and set CY flag
            state->cc.cy = rightmost == 1;
            // set left-most bit to what the right-most bit was
            state->a = (state->a >> 1) | (rightmost << 7);
        }
            break;
        case 0x10: 
            unused_opcode(state, *opcode);
            break;
        case 0x11:  // D <- byte 3, E <- byte 2
        { 
            state->d = opcode[2];
            state->e = opcode[1];
            state->pc += 2;
        }
            break;
        case 0x12:  // STAX D: (DE) <- A
        {
            uint16_t offset = makeword(state->d, state->e);
            mem_write(state, offset, state->a);
        }
            break;
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
        case 0x16:  // MVI D,D8: D <- byte 2
            state->d = next_byte(state);
            break;
        case 0x17:  // RAL: A = A << 1; bit 0 = prev CY; CY = prev bit 7
        {
            // Rotate Accumulator Left Through Carry
            // CY A
            // 0  10110101
            // =>
            // CY A
            // 1  01101010
            uint8_t leftmost = state->a >> 7;
            uint8_t prev_cy = state->cc.cy;

            state->cc.cy = leftmost;
            state->a = (state->a << 1) | prev_cy;
        }
            break;
        case 0x18:
            unused_opcode(state, *opcode);
            break;
        case 0x19:  // DAD D: HL = HL + DE
        {
            dad_xy(state, &state->d, &state->e);
        }
            break;
        case 0x1a:  // LDAX D
        {
            state->a = get_de_mem(state);
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
        case 0x1e:  // E <- byte 2
            state->e = next_byte(state);
            break;
        case 0x1f:  // RAR
        {
            // Rotate Accumulator Right Through Carry
            // A        CY
            // 01101010 1
            // =>
            // A        CY
            // 10110101 0
            uint8_t rightmost = state->a & 1;
            uint8_t prev_cy = state->cc.cy;
            state->cc.cy = rightmost;
            state->a = (state->a >> 1) | (prev_cy << 7);
        }
            break;
        case 0x20:
            unused_opcode(state, *opcode);
            break;
        case 0x21:  // LXI H,D16: H <- byte 3, L <- byte 2
        {
            state->h = opcode[2];
            state->l = opcode[1];
            state->pc += 2;
        }
            break;
        case 0x22:  // SHLD adr: (adr) <-L; (adr+1)<-H
        {
            // the following two opcodes form an address
            // when put together
            uint16_t addr = makeword(opcode[2], opcode[1]);
            mem_write(state, addr, state->l);
            mem_write(state, addr + 1, state->h);
            state->pc += 2;
        }
            break;
        case 0x23:  // INX H
            inx_xy(&state->h, &state->l);
            break;
        case 0x24:  // INR H
            inr_x(state, &state->h);
            break;
        case 0x25:
            dcr_x(state, &state->h);
            break;
        case 0x26:  // MVI H,D8
            state->h = next_byte(state);
            break;
        case 0x27:  // DAA - decimal adjust accumulator
        // The eight-bit number in the accumulator
        // is adjusted to form two four-bi 
        // Binary-Coded-Decimal digits by the
        // following process:
        // 1. If the value of the least significant
        // 4 bits of the accumulator is greater
        // than 9 or if the AC flag is set, 6 is
        // added to the accumulator.
        // 2. If the value of the most significant
        // 4 bits of the accumulator is now greater
        // than 9, or if the CY flag is set, 6 is
        // added to the most significant 4 bits
        // of the accumulator.
        {
            uint8_t least4, most4;
            uint16_t answer;
            // 1.
            least4 = state->a & 0xf;
            if (least4 > 9 || state->cc.ac) {
                answer = state->a + 6;
                // set flags of intermediate result
                set_arith_flags(state, answer, SET_ALL_FLAGS);
                state->a = answer & 0xff;
            }
            // 2.
            most4 = state->a >> 4;
            if (most4 > 9 || state->cc.cy) {
                most4 += 6;
            }
            // put most and least sig. 4 digits back
            // together
            answer = (most4 << 4) | least4;
            set_arith_flags(state, answer, SET_ALL_FLAGS);
            state->a = answer & 0xff;
        }
            break;
        case 0x28: 
            unused_opcode(state, *opcode); 
            break;
        case 0x29:  // DAD H
        {
            dad_xy(state, &state->h, &state->l);
        }
            break;
        case 0x2a:  // LHLD adr
        {
            // get address (16 bits)
            uint16_t addr = makeword(opcode[2], opcode[1]);
            uint8_t lo, hi;
            lo = state->memory[addr];
            hi = state->memory[addr + 1]; 
            state->l = lo;
            state->h = hi;
            state->pc += 2;
        }
            break;
        // page 4-8 of the manual
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
        case 0x2e:  // MVI L,D8
            // L <- byte 2
            state->l = next_byte(state);
            break;
        case 0x2f:  // CMA: A <- !A
        {
            // complement accumulator
            state->a = ~state->a;
            // no flags affected
        }
            break;
        case 0x30:
            unused_opcode(state, *opcode);
            break;
        case 0x31:  // LXI SP, D16
        {
            // SP.hi <- byte 3, SP>lo <- byte 2
            // SP is 16 bits
            // I think hi = most significant bits
            uint8_t byte2, byte3;
            byte3 = opcode[2];
            byte2 = opcode[1];
            state->sp = makeword(byte3, byte2);
            state->pc += 2;
        }
            break;
        case 0x32:  // STA adr
        {
            // (adr) <- A
            // store accumulator direct
            // uint16_t addr = makeword(opcode[2], opcode[1]);
            uint16_t addr = next_word(state);
            mem_write(state, addr, state->a);
            // state->pc += 2;
        }
            break;
        case 0x33:  // INX SP: SP <- SP + 1
            // stack pointer is already 16 bits
            state->sp++; 
            break;
        case 0x34:  // INR M
            inr_x(state, get_hl_ptr(state));
            break;
        case 0x35:  // DCR M
        {
            uint16_t offset = hl_addr(state);
            uint8_t *m_ptr = &state->memory[offset];
            dcr_x(state, m_ptr);
            // dcr_x(state, get_hl_ptr(state));
        }
            break;
        case 0x36:  // (HL) <- byte 2
            set_hl_mem(state, next_byte(state));
            break;
        case 0x37:  // STC
            // set carry flag to 1
            state->cc.cy = 1;
            break;
        case 0x38: 
            unused_opcode(state, *opcode); 
            break;
        case 0x39:  // DAD SP
        {
            // uglier implementation
            uint32_t answer;
            answer = tworeg_add(
                &state->h, &state->l, state->sp);
            state->cc.cy = ((answer & 0xffff0000) != 0);
        }
            break;
        case 0x3a:  // LDA adr
        {
            // A <- (adr)
            // uint16_t addr = makeword(opcode[2], opcode[1]);
            // state->a = mem_read(state, addr);
            // state->pc += 2;
            state->a = mem_read(state, next_word(state));
        }
            break;
        case 0x3b:  // DCX SP
        {
            uint16_t curr_sp = state->sp;
            state->sp = curr_sp - 1;
            // no flags set
        }
            break;
        case 0x3c:  // INR A
            inr_x(state, &state->a);
            break;
        case 0x3d:
            dcr_x(state, &state->a);
            break;
        case 0x3e:  // MVI A,D8
            // A <- byte 2
            state->a = next_byte(state);
            break;
        case 0x3f:  // CMC: CY = !CY
            state->cc.cy = ~state->cc.cy;
            break;
        case 0x40:  // MOV B,B
            // I think this is redundant, but including
            // it here anyway
            state->b = state->b;
            break;
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
        case 0x46:  // B <- (HL)
            state->b = get_hl_mem(state);
            break;
        case 0x47: 
            state->b = state->a;
            break;
        case 0x48:
            state->c = state->b;
            break;
        case 0x49:
            state->c = state->c;
            break;
        case 0x4a:
            state->c = state->d;
            break;
        case 0x4b:
            state->c = state->e;
            break;
        case 0x4c:
            state->c = state->h;
            break;
        case 0x4d:
            state->c = state->l;
            break;
        case 0x4e:
            state->c = get_hl_mem(state);
            break;
        case 0x4f:
            state->c = state->a;
            break;
        case 0x50:
            state->d = state->b;
            break;
        case 0x51:
            state->d = state->c;
            break;
        case 0x52:
            state->d = state->d;
            break;
        case 0x53:
            state->d = state->e;
            break;
        case 0x54:
            state->d = state->h;
            break;
        case 0x55:
            state->d = state->l;
            break;
        case 0x56:
            state->d = get_hl_mem(state);
            break;
        case 0x57:
            state->d = state->a;
            break;
        case 0x58:  // MOV E,B
            state->e = state->b;
            break;
        case 0x59:
            state->e = state->c;
            break;
        case 0x5a:
            state->e = state->d;
            break;
        case 0x5b:
            state->e = state->e;
            break;
        case 0x5c:
            state->e = state->h;
            break;
        case 0x5d:
            state->e = state->l;
            break;
        case 0x5e:
            state->e = get_hl_mem(state);
            break;
        case 0x5f:
            state->e = state->a;
            break;
        case 0x60:  // MOV H,B
            state->h = state->b;
            break;
        case 0x61:
            state->h = state->c;
            break;
        case 0x62:
            state->h = state->d;
            break;
        case 0x63:
            state->h = state->e;
            break;
        case 0x64:
            state->h = state->h;
            break;
        case 0x65:
            state->h = state->l;
            break;
        case 0x66:
            state->h = get_hl_mem(state);
            break;
        case 0x67:
            state->h = state->a;
            break;
        case 0x68:
            state->l = state->b;
            break;
        case 0x69:
            state->l = state->c;
            break;
        case 0x6a:
            state->l = state->d;
            break;
        case 0x6b:
            state->l = state->e;
            break;
        case 0x6c:
            state->l = state->h;
            break;
        case 0x6d:
            state->l = state->l;
            break;
        case 0x6e:
            state->l = get_hl_mem(state);
            break;
        case 0x6f:
            state->l = state->a;
            break;
        case 0x70: // MOV M,B
            set_hl_mem(state, state->b);
            break;
        case 0x71:
            set_hl_mem(state, state->c);
            break;
        case 0x72:
            set_hl_mem(state, state->d);
            break;
        case 0x73:
            set_hl_mem(state, state->e);
            break;
        case 0x74:
            set_hl_mem(state, state->h);
            break;
        case 0x75:
            set_hl_mem(state, state->l);
            break;
        case 0x76: 
            // HLT (Halt) instruction
            printf("Halting execution...\n");
            exit(0);
            break;
        case 0x77:
            set_hl_mem(state, state->a);
            break;
        case 0x78:
            state->a = state->b;
            break;
        case 0x79:
            state->a = state->c;
            break;
        case 0x7a:
            state->a = state->d;
            break;
        case 0x7b:
            state->a = state->e;
            break;
        case 0x7c:
            state->a = state->h;
            break;
        case 0x7d:
            state->a = state->l;
            break;
        case 0x7e:
            state->a = get_hl_mem(state);
            break;
        case 0x7f:  // MOV A,A
            state->a = state->a;
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
            uint8_t m = get_hl_mem(state);
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
            uint8_t m = get_hl_mem(state);
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
            uint8_t m = get_hl_mem(state);
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
            uint8_t m = get_hl_mem(state);
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
            uint8_t m = get_hl_mem(state);
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
            uint8_t m = get_hl_mem(state);
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
            uint8_t m = get_hl_mem(state);
            ora_x(state, m);
        }
            break;
        case 0xb7:
        {
            ora_x(state, state->a);
        }
            break;
        case 0xb8:  // CMP B
        {
            cmp_x(state, state->b);
        }
            break;
        case 0xb9:
        {
            cmp_x(state, state->c);
        }
            break;
        case 0xba:
        {
            cmp_x(state, state->d);
        }
            break;
        case 0xbb:
        {
            cmp_x(state, state->e);
        }
            break;
        case 0xbc:
        {
            cmp_x(state, state->h);
        }
            break;
        case 0xbd:
        {
            cmp_x(state, state->l);
        }
            break;
        case 0xbe:
        {
            cmp_x(state, get_hl_mem(state));
        }
            break;
        case 0xbf:
        {
            cmp_x(state, state->a);
        }
            break;
        case 0xc0:  // RNZ
        {
            // if NZ, RET
            uint8_t not_zero = !state->cc.z;
            if (not_zero) {
                ret(state);
            }
        }
            break;
        case 0xc1:  // POP B
        {
            // pop the stack into
            // registers B and C
            pop(state, &state->b, &state->c);
        }
            break;
        case 0xc2:  // JNZ adr
        {
            uint8_t notzero = state->cc.z == 0;
            uint16_t adr = makeword(opcode[2], opcode[1]);
            jmp_cond(state, adr, notzero);
        }
            break;
        case 0xc3:  // JMP adr
        {
            uint16_t adr = makeword(opcode[2], opcode[1]);
            jmp(state, adr);
        }
            break;
        case 0xc4:  // CNZ adr
        {
            uint8_t notzero = !state->cc.z; 
            uint16_t adr = makeword(opcode[2], opcode[1]);
            call_cond(state, adr, notzero);
        }
            break;
        case 0xc5:  // PUSH B
        {
            push_x(state, state->b, state->c);
        }
            break;
        case 0xc6:  // ADI D8
        {
            // The immediate form is the almost the 
            // same except the source of the addend 
            // is the byte after the instruction. 
            // Since "opcode" is a pointer to the 
            // current instruction in memory, 
            // opcode[1] will be the immediately following byte.
            uint16_t answer;
            answer = (uint16_t) state->a + (uint16_t) opcode[1];
            set_arith_flags(state, answer, SET_ALL_FLAGS);

            state->a = (uint8_t) answer;
            // instruction is of size 2
            state->pc += 1;
        }
            break;
        case 0xc7:  // RST 0
        {
            call_adr(state, 0);
        }
            break;
        case 0xc8:  // RZ
        {
            // if Z, RET
            if (state->cc.z) {
                ret(state);
            }
        }
            break;
        case 0xc9:  // RET
        {
            ret(state);
        }
            break;
        case 0xca:  // JZ adr
        {
            uint16_t adr = makeword(opcode[2], opcode[1]);
            jmp_cond(state, adr, state->cc.z);
        }
            break;
        case 0xcb:
        {
            unused_opcode(state, *opcode);
        }
            break;
        case 0xcc:  // CZ adr
        {
            uint16_t adr = makeword(opcode[2], opcode[1]);
            call_cond(state, adr, state->cc.z);
        }
            break;
        case 0xcd:  // CALL adr
        {
            uint16_t adr = makeword(opcode[2], opcode[1]);
            call_adr(state, adr); 
        }
            break;
        case 0xce:  // ACI D8: A <- A + data + CY
        {
            uint8_t data = opcode[1];
            uint16_t a, answer;
            a = (uint16_t) state->a;
            answer = a + data + state->cc.cy;
            set_arith_flags(state, answer, SET_ALL_FLAGS);
            state->a = answer & 0xff;
            state->pc += 1;
        }
            break;
        case 0xcf: // RST 8
        {
            call_adr(state, 8);
        }
            break;
        case 0xd0:  // RNC
        {
            // if not carry, return
            if (!state->cc.cy) {
                ret(state);
            }
        } 
            break;
        case 0xd1:
        {
            pop(state, &state->d, &state->e);
        }
            break;
        case 0xd2:  // JNC adr
        {
            // if not carry, jmp
            uint16_t adr = makeword(opcode[2], opcode[1]);
            jmp_cond(state, adr, !state->cc.cy);
        }
            break;
        case 0xd3:  // OUT D8
        {
            io->out = 1;
            io->port = opcode[1];
            io->value = state->a;

            // skip over data byte
            state->pc += 1;
        }
            break;
        case 0xd4:
        {
            uint8_t nocarry = !state->cc.cy;
            uint16_t adr = makeword(opcode[2], opcode[1]);
            call_cond(state, adr, nocarry);
        }
            break;
        case 0xd5:  // PUSH D
        {
            push_x(state, state->d, state->e);
        }
            break;
        case 0xd6:   // SUI D8
        {
            uint8_t byte = opcode[1];
            uint16_t answer = (uint16_t) state->a - (uint16_t) byte;
            set_arith_flags(state, answer, 
                SET_ALL_FLAGS ^ SET_CY_FLAG);
            if (byte > state->a) {
                state->cc.cy = 1;
            }
            state->a = answer & 0xff;
            
            state->pc += 1;
        } 
            break;
        case 0xd7:  // CALL 10 (16 in decimal)
        {
            // 0, 8, 16, 24, 32, 40, 48, and 56
            call_adr(state, 16);
        }
            break;
        case 0xd8:  // RC
        {
            if (state->cc.cy) {
                ret(state);
            }
        }
            break;
        case 0xd9:
            unused_opcode(state, *opcode);
            break;
        case 0xda:
        {
            uint16_t adr = makeword(opcode[2], opcode[1]);
            jmp_cond(state, adr, state->cc.cy);
        }
            break;
        case 0xdb:  // IN D8
        {
            // if (state->input) {
            //     state->a = state->input(opcode[1]);
            // }
            io->in = 1;
            io->port = opcode[1];
            // skip over data byte
            state->pc++;
        }
            break;
        case 0xdc:  // CC adr
        {
            uint16_t adr = makeword(opcode[2], opcode[1]);
            call_cond(state, adr, state->cc.cy);
        }
            break;
        case 0xdd:
            unused_opcode(state, *opcode); 
            break;
        case 0xde:  // SBI D8
        {
            uint16_t answer, a, cy, byte;
            a = (uint16_t) state->a;
            cy = (uint16_t) state->cc.cy;
            byte = (uint16_t) opcode[1];
            answer = a - byte - cy;
            set_arith_flags(state, answer, 
                SET_ALL_FLAGS ^ SET_CY_FLAG);
            // set CY if subtracting larger num
            if (byte + cy > a) {
                state->cc.cy = 1;
            }
            state->a = answer & 0xff;
            state->pc += 2;
        }
            break;
        case 0xdf:
        {
            call_adr(state, 24);
        }
            break;
        case 0xe0:  // RPO
        {
            // if parity odd, RET
            if (!state->cc.p) {
                ret(state);
            }
        }
            break;
        case 0xe1:  // POP H
        {
            pop(state, &state->h, &state->l);
        }
            break;
        case 0xe2:  // JPO adr
        {
            uint16_t adr = makeword(opcode[2], opcode[1]);
            jmp_cond(state, adr, !state->cc.p);
        }
            break;
        case 0xe3:  // XTHL
        {
            // L <-> (SP); H <-> (SP+1)
            uint16_t sp = state->sp;
            uint8_t *sp_h, *sp_l;
            sp_h = &state->memory[sp + 1];
            sp_l = &state->memory[sp]; 
            swp_ptrs(&state->l, sp_l);
            swp_ptrs(&state->h, sp_h);
        }
            break;
        case 0xe4:  // CPO adr
        {
            uint8_t odd = !state->cc.p;
            uint16_t adr = makeword(opcode[2], opcode[1]);
            call_cond(state, adr, odd);
        }
            break;
        case 0xe5:  // PUSH H
        {
            push_x(state, state->h, state->l);
        }
            break;
        case 0xe6:  // ANI D8
        {
            uint8_t answer = state->a & opcode[1];
            set_logic_flags(state, answer, SET_ALL_FLAGS);
            state->a = answer & 0xff;
            state->pc += 1;
        }
            break;
        case 0xe7:
        {
            // decimal value = 32
            call_adr(state, 0x20);
        }
            break;
        case 0xe8:  // RPE
        {
            if (state->cc.p) {
                ret(state);
            }
        }
            break;
        case 0xe9:  // PCHL
        {
            // PC.hi <- H; PC.lo <- L
            state->pc = makeword(state->h, state->l);
        }
            break;
        case 0xea:  // JPE adr
        {
            // jmp if even
            uint16_t adr = makeword(opcode[2], opcode[1]);
            jmp_cond(state, adr, state->cc.p);
        }
            break;
        case 0xeb:  // XCHG
        {
            // H <-> D; L <-> E
            swp_ptrs(&state->h, &state->d);
            swp_ptrs(&state->l, &state->e);
        }
            break;
        case 0xec:  // CPE adr
        {
            // call address if parity even
            uint16_t adr = makeword(opcode[2], opcode[1]);
            call_cond(state, adr, state->cc.p);
        }
            break;
        case 0xed:
            unused_opcode(state, *opcode);
            break;
        case 0xee:  // XRI D8
        {
            uint16_t answer;
            answer = (uint16_t) state->a ^ opcode[1];
            set_logic_flags(state, answer,
                SET_ALL_FLAGS);
            state->a = answer & 0xff;
            state->pc += 1;
        }
            break;
        case 0xef:  // RST
        {
            call_adr(state, 0x28);
        }
            break;
        case 0xf0:  // RP
        {
            // if positive, RET
            if (state->cc.s == 0) {
                ret(state);
            }
        }
        case 0xf1:  // POP PSW
        {
            uint16_t sp_addr = state->sp;
            uint8_t sp_val = state->memory[sp_addr];

            // (CY) <- ((SP))O
            state->cc.cy = sp_val & 1;

            // (P) <- ((SP))2
            state->cc.p = (sp_val & (1 << 2)) > 0;

            // (AC) <- ((SP))4
            state->cc.ac = (sp_val & (1 << 4)) > 0;

            // (Z) <- ((SP))6
            state->cc.z = (sp_val & (1 << 6)) > 0;

            // (S) <- ((SP))7
            state->cc.s = (sp_val & (1 << 7)) > 0;

            // (A) <- ((SP) +1)
            state->a = state->memory[sp_addr + 1];

            // (SP) <- (SP) + 2
            state->sp += 2;

            // below is the implementation from
            // the site...
            // uint8_t psw = state->memory[sp_addr];

            // state->cc.z = (0x01 == (psw & 0x01);
            // state->cc.s = (0x02 == (psw & 0x02);
            // state->cc.p = (0x04 == (psw & 0x04);
            // state->cc.cy = (0x05 == (psw & 0x05);
            // state->cc.ac = (0x10 == (psw & 0x10);
            //
            // I don't think it matters as long as
            // the flags match when pushed and popped
        }
            break;
        case 0xf2:  // JP adr
        {
            // if positive, JMP
            uint16_t adr = makeword(opcode[2], opcode[1]);
            jmp_cond(state, adr, state->cc.s == 0);
        }
            break;
        case 0xf3:  // DI
        {
            // disable interrupts
            state->int_enable = 0;
        }
            break;
        case 0xf4:   // CP adr
        {
            uint8_t pos = !state->cc.s;
            uint16_t adr = makeword(opcode[2], opcode[1]);
            call_cond(state, adr, pos);
        }
            break;
        case 0xf5:  // PUSH PSW
        {
            uint16_t sp_adr = state->sp;
            
            // ((SP) - 1) <- A
            mem_write(state, sp_adr - 1, state->a);

            uint8_t sp_flags = 0;

            // ((SP) - 2)0 <- CY
            sp_flags |= state->cc.cy; 

            // (........)1 <- 1
            sp_flags |= (1 << 1);

            // (........)2 <- P
            sp_flags |= (state->cc.p << 2);

            // (........)3 <- 0
 
            // (........)4 <- AC
            sp_flags |= (state->cc.ac << 4);

            // (........)5 <- 0

            // (........)6 <- Z
            sp_flags |= (state->cc.z << 6);

            // (........)7 <- S
            sp_flags |= (state->cc.s << 7);
            mem_write(state, sp_adr - 2, sp_flags);

            // (SP) <- (SP) - 2
            state->sp -= 2;
        } 
            break;
        case 0xf6:  // ORI D8
        {
            uint16_t answer;
            answer = (uint16_t) state->a | opcode[1];
            set_logic_flags(state, answer,
                SET_ALL_FLAGS);
            state->a = answer & 0xff;
            state->pc += 1;
        }
            break;
        case 0xf7:  // RST 6 (CALL $30)
        {
            call_adr(state, 0x30);
        }
            break;
        case 0xf8:  // RM
        {
            // if minus, RET
            if (state->cc.s) {
                ret(state);
            }
        }
            break;
        case 0xf9:  // SPHL: SP = HL
        {
            state->sp = makeword(state->h, state->l);
        }
            break;
        case 0xfa:  // JM
        {
            // jump if sign is negative (sign = 1)
            uint16_t adr = makeword(opcode[2], opcode[1]);
            jmp_cond(state, adr, state->cc.s);
        }
            break;
        case 0xfb:  // EI
        {
            // enable interrupts
            state->int_enable = 1;
        }
            break;
        case 0xfc:  // CM adr
        {
            // if minus, call
            uint8_t minus = state->cc.s;
            uint16_t adr = makeword(opcode[2], opcode[1]);
            call_cond(state, adr, minus);
        }
            break;
        case 0xfd:
            unused_opcode(state, *opcode);
            break;
        case 0xfe:  // CPI byte
        {
            cmp_x(state, opcode[1]);
            state->pc += 1;
        }
            break;
        case 0xff:
        {
            call_adr(state, 0x38);
        }
            break;
    }

    return cycles[*opcode];
}
