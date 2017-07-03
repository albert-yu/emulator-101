#include <stdint.h>

typedef struct ConditionCodes
{
    uint8_t     z:1;  // z occupies 1 bit in the struct
    uint8_t     s:1;
    uint8_t     p:1;
    uint8_t     cy:1;
    uint8_t     ac:1;
    uint8_t     pad:3;
} ConditionCodes;

typedef struct State8080
{
    uint8_t     a;
    uint8_t     b;
    uint8_t     c;
    uint8_t     d;
    uint8_t     e;
    uint8_t     h;
    uint8_t     l;
    uint16_t    sp;
    uint16_t    pc;
    uint8_t     *memory;
    struct      ConditionCodes      cc;
    uint8_t     int_enable;
} State8080;


void UnimplementedInstruction(State8080 *state)
{
    printf("Error: Unimplemented instruction\n");
    exit(1);
}

// Flags

uint8_t Zero(uint16_t answer)
{
    // set to 1 if answer is 0, 0 otherwise
    return ((answer & 0xff) == 0);
}

uint8_t Sign(uint16_t answer)
{
    // set to 1 when bit 7 of the math instruction is set
    return ((answer & 0x80) == 0);
}

uint8_t Parity(uint16_t answer)
{
    uint8_t ans8bit = answer & 0xff;
    // 1 (true) if even, 0 otherwise
    return ((ans8bit & 0x01) == 0); 
}

uint8_t Carry(uint16_t answer)
{
    // set to 1 when instruction resulted in a carry or borrow into the high order bit
    return (answer > 0xff); 
}

uint8_t AuxCarry(uint16_t answer)
{
    // skip implementation because Space Invaders doesn't use it
    return 0;  
}

void EmulateOp(State8080 *state)
{
    unsigned char *opcode = &state->memory[state->pc];

    switch(*opcode)
    {
        case 0x00:  // NOP
            break;
        case 0x01: 
            state->c = opcode[1];  // c <- byte 2
            state->b = opcode[2];  // b <- byte 3
            state->pc += 2;  // advance two more bytes
            break;
        case 0x02: UnimplementedInstruction(state); break;
        case 0x03: UnimplementedInstruction(state); break;
        case 0x04: 
            uint16_t answer = (uint16_t) state->b + 1;
            state->cc.z = Zero(answer);
            state->cc.s = Sign(answer);
            state->cc.p = Parity(answer); // 1 (true) if even, 0 otherwise
            state->cc.ac = AuxCarry(answer); 
            state->b = answer & 0xff;  // b <- b + 1
            break;
        case 0x05: 
            uint8_t answer = state->b - 1;
            state->cc.z = Zero(answer);
            state->cc.s = Sign(answer);
            state->cc.p = Parity(answer);
            state->cc.ac = AuxCarry(answer); 
            state->b = answer & 0xff;
            break;
        case 0x06: 
            state->b = opcode[1];  // b <- byte 2
            state->pc += 1;
            break;
        case 0x07:  // A = A << 1; bit 0 = prev bit 7; CY = prev bit 7
            // get left-most bit
            uint8_t leftmost = state->a & 0x80;
            state->cc.cy = leftmost;
            // TODO: Set right-most bit to whatever the left-most bit is
            break;
        case 0x08:  // NOP
            break;
        case 0x09: UnimplementedInstruction(state); break;
        case 0x0a: UnimplementedInstruction(state); break;
        case 0x0b: UnimplementedInstruction(state); break;
        case 0x0c: 
            uint16_t answer = ((uint16_t) state->c) + 1; 
            break;
        case 0x0d: UnimplementedInstruction(state); break;
        case 0x0e: UnimplementedInstruction(state); break;
        case 0x0f: UnimplementedInstruction(state); break;
        case 0x10: UnimplementedInstruction(state); break;
        case 0x11: UnimplementedInstruction(state); break;
        case 0x12: UnimplementedInstruction(state); break;
        case 0x13: UnimplementedInstruction(state); break;
        case 0x14: UnimplementedInstruction(state); break;
        case 0x15: UnimplementedInstruction(state); break;
        case 0x16: UnimplementedInstruction(state); break;
        case 0x17: UnimplementedInstruction(state); break;
        case 0x18: UnimplementedInstruction(state); break;
        case 0x19: UnimplementedInstruction(state); break;
        case 0x1a: UnimplementedInstruction(state); break;
        case 0x1b: UnimplementedInstruction(state); break;
        case 0x1c: UnimplementedInstruction(state); break;
        case 0x1d: UnimplementedInstruction(state); break;
        case 0x1e: UnimplementedInstruction(state); break;
        case 0x1f: UnimplementedInstruction(state); break;
        case 0x20: UnimplementedInstruction(state); break;
        case 0x21: UnimplementedInstruction(state); break;
        case 0x22: UnimplementedInstruction(state); break;
        case 0x23: UnimplementedInstruction(state); break;
        case 0x24: UnimplementedInstruction(state); break;
        case 0x25: UnimplementedInstruction(state); break;
        case 0x26: UnimplementedInstruction(state); break;
        case 0x27: UnimplementedInstruction(state); break;
        case 0x28: UnimplementedInstruction(state); break;
        case 0x29: UnimplementedInstruction(state); break;
        case 0x2a: UnimplementedInstruction(state); break;
        case 0x2b: UnimplementedInstruction(state); break;
        case 0x2c: UnimplementedInstruction(state); break;
        case 0x2d: UnimplementedInstruction(state); break;
        case 0x2e: UnimplementedInstruction(state); break;
        case 0x2f: UnimplementedInstruction(state); break;
        case 0x30: UnimplementedInstruction(state); break;
        case 0x31: UnimplementedInstruction(state); break;
        case 0x32: UnimplementedInstruction(state); break;
        case 0x33: UnimplementedInstruction(state); break;
        case 0x34: UnimplementedInstruction(state); break;
        case 0x35: UnimplementedInstruction(state); break;
        case 0x36: UnimplementedInstruction(state); break;
        case 0x37: UnimplementedInstruction(state); break;
        case 0x38: UnimplementedInstruction(state); break;
        case 0x39: UnimplementedInstruction(state); break;
        case 0x3a: UnimplementedInstruction(state); break;
        case 0x3b: UnimplementedInstruction(state); break;
        case 0x3c: UnimplementedInstruction(state); break;
        case 0x3d: UnimplementedInstruction(state); break;
        case 0x3e: UnimplementedInstruction(state); break;
        case 0x3f: UnimplementedInstruction(state); break;
        case 0x40: UnimplementedInstruction(state); break;
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
            state->b = state->m;
            break;
        case 0x47: 
            state->b = state->a;
            break;
        case 0x48: UnimplementedInstruction(state); break;
        case 0x49: UnimplementedInstruction(state); break;
        case 0x4a: UnimplementedInstruction(state); break;
        case 0x4b: UnimplementedInstruction(state); break;
        case 0x4c: UnimplementedInstruction(state); break;
        case 0x4d: UnimplementedInstruction(state); break;
        case 0x4e: UnimplementedInstruction(state); break;
        case 0x4f: UnimplementedInstruction(state); break;
        case 0x50: UnimplementedInstruction(state); break;
        case 0x51: UnimplementedInstruction(state); break;
        case 0x52: UnimplementedInstruction(state); break;
        case 0x53: UnimplementedInstruction(state); break;
        case 0x54: UnimplementedInstruction(state); break;
        case 0x55: UnimplementedInstruction(state); break;
        case 0x56: UnimplementedInstruction(state); break;
        case 0x57: UnimplementedInstruction(state); break;
        case 0x58: UnimplementedInstruction(state); break;
        case 0x59: UnimplementedInstruction(state); break;
        case 0x5a: UnimplementedInstruction(state); break;
        case 0x5b: UnimplementedInstruction(state); break;
        case 0x5c: UnimplementedInstruction(state); break;
        case 0x5d: UnimplementedInstruction(state); break;
        case 0x5e: UnimplementedInstruction(state); break;
        case 0x5f: UnimplementedInstruction(state); break;
        case 0x60: UnimplementedInstruction(state); break;
        case 0x61: UnimplementedInstruction(state); break;
        case 0x62: UnimplementedInstruction(state); break;
        case 0x63: UnimplementedInstruction(state); break;
        case 0x64: UnimplementedInstruction(state); break;
        case 0x65: UnimplementedInstruction(state); break;
        case 0x66: UnimplementedInstruction(state); break;
        case 0x67: UnimplementedInstruction(state); break;
        case 0x68: UnimplementedInstruction(state); break;
        case 0x69: UnimplementedInstruction(state); break;
        case 0x6a: UnimplementedInstruction(state); break;
        case 0x6b: UnimplementedInstruction(state); break;
        case 0x6c: UnimplementedInstruction(state); break;
        case 0x6d: UnimplementedInstruction(state); break;
        case 0x6e: UnimplementedInstruction(state); break;
        case 0x6f: UnimplementedInstruction(state); break;
        case 0x70: UnimplementedInstruction(state); break;
        case 0x71: UnimplementedInstruction(state); break;
        case 0x72: UnimplementedInstruction(state); break;
        case 0x73: UnimplementedInstruction(state); break;
        case 0x74: UnimplementedInstruction(state); break;
        case 0x75: UnimplementedInstruction(state); break;
        case 0x76: UnimplementedInstruction(state); break;
        case 0x77: UnimplementedInstruction(state); break;
        case 0x78: UnimplementedInstruction(state); break;
        case 0x79: UnimplementedInstruction(state); break;
        case 0x7a: UnimplementedInstruction(state); break;
        case 0x7b: UnimplementedInstruction(state); break;
        case 0x7c: UnimplementedInstruction(state); break;
        case 0x7d: UnimplementedInstruction(state); break;
        case 0x7e: UnimplementedInstruction(state); break;
        case 0x7f: UnimplementedInstruction(state); break;
        case 0x80:  // ADD B
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->b;    
            state->cc.z = Zero(answer);    
            state->cc.s = Sign(answer);    
            state->cc.cy = Carry(answer);    
            state->cc.p = Parity(answer); 
            state->a = answer & 0xff;
            break;
        case 0x81:  // ADD C
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->c;    
            state->cc.z = Zero(answer);    
            state->cc.s = Sign(answer);    
            state->cc.cy = Carry(answer);    
            state->cc.p = Parity(answer);    
            state->a = answer & 0xff;
            break;
        case 0x82: 
            uint16_t answer = (uint16_t) state->a + (uint16_t) state->d;    
            state->cc.z = Zero(answer);    
            state->cc.s = Sign(answer);    
            state->cc.cy = Carry(answer);    
            state->cc.p = Parity(answer & 0xff);    
            state->a = answer & 0xff;
            break;
        case 0x83: UnimplementedInstruction(state); break;
        case 0x84: UnimplementedInstruction(state); break;
        case 0x85: UnimplementedInstruction(state); break;
        case 0x86: UnimplementedInstruction(state); break;
        case 0x87: UnimplementedInstruction(state); break;
        case 0x88: UnimplementedInstruction(state); break;
        case 0x89: UnimplementedInstruction(state); break;
        case 0x8a: UnimplementedInstruction(state); break;
        case 0x8b: UnimplementedInstruction(state); break;
        case 0x8c: UnimplementedInstruction(state); break;
        case 0x8d: UnimplementedInstruction(state); break;
        case 0x8e: UnimplementedInstruction(state); break;
        case 0x8f: UnimplementedInstruction(state); break;
        case 0x90: UnimplementedInstruction(state); break;
        case 0x91: UnimplementedInstruction(state); break;
        case 0x92: UnimplementedInstruction(state); break;
        case 0x93: UnimplementedInstruction(state); break;
        case 0x94: UnimplementedInstruction(state); break;
        case 0x95: UnimplementedInstruction(state); break;
        case 0x96: UnimplementedInstruction(state); break;
        case 0x97: UnimplementedInstruction(state); break;
        case 0x98: UnimplementedInstruction(state); break;
        case 0x99: UnimplementedInstruction(state); break;
        case 0x9a: UnimplementedInstruction(state); break;
        case 0x9b: UnimplementedInstruction(state); break;
        case 0x9c: UnimplementedInstruction(state); break;
        case 0x9d: UnimplementedInstruction(state); break;
        case 0x9e: UnimplementedInstruction(state); break;
        case 0x9f: UnimplementedInstruction(state); break;
        case 0xa0: UnimplementedInstruction(state); break;
        case 0xa1: UnimplementedInstruction(state); break;
        case 0xa2: UnimplementedInstruction(state); break;
        case 0xa3: UnimplementedInstruction(state); break;
        case 0xa4: UnimplementedInstruction(state); break;
        case 0xa5: UnimplementedInstruction(state); break;
        case 0xa6: UnimplementedInstruction(state); break;
        case 0xa7: UnimplementedInstruction(state); break;
        case 0xa8: UnimplementedInstruction(state); break;
        case 0xa9: UnimplementedInstruction(state); break;
        case 0xaa: UnimplementedInstruction(state); break;
        case 0xab: UnimplementedInstruction(state); break;
        case 0xac: UnimplementedInstruction(state); break;
        case 0xad: UnimplementedInstruction(state); break;
        case 0xae: UnimplementedInstruction(state); break;
        case 0xaf: UnimplementedInstruction(state); break;
        case 0xb0: UnimplementedInstruction(state); break;
        case 0xb1: UnimplementedInstruction(state); break;
        case 0xb2: UnimplementedInstruction(state); break;
        case 0xb3: UnimplementedInstruction(state); break;
        case 0xb4: UnimplementedInstruction(state); break;
        case 0xb5: UnimplementedInstruction(state); break;
        case 0xb6: UnimplementedInstruction(state); break;
        case 0xb7: UnimplementedInstruction(state); break;
        case 0xb8: UnimplementedInstruction(state); break;
        case 0xb9: UnimplementedInstruction(state); break;
        case 0xba: UnimplementedInstruction(state); break;
        case 0xbb: UnimplementedInstruction(state); break;
        case 0xbc: UnimplementedInstruction(state); break;
        case 0xbd: UnimplementedInstruction(state); break;
        case 0xbe: UnimplementedInstruction(state); break;
        case 0xbf: UnimplementedInstruction(state); break;
        case 0xc0: UnimplementedInstruction(state); break;
        case 0xc1: UnimplementedInstruction(state); break;
        case 0xc2: UnimplementedInstruction(state); break;
        case 0xc3: UnimplementedInstruction(state); break;
        case 0xc4: UnimplementedInstruction(state); break;
        case 0xc5: UnimplementedInstruction(state); break;
        case 0xc6: UnimplementedInstruction(state); break;
        case 0xc7: UnimplementedInstruction(state); break;
        case 0xc8: UnimplementedInstruction(state); break;
        case 0xc9: UnimplementedInstruction(state); break;
        case 0xca: UnimplementedInstruction(state); break;
        case 0xcb: UnimplementedInstruction(state); break;
        case 0xcc: UnimplementedInstruction(state); break;
        case 0xcd: UnimplementedInstruction(state); break;
        case 0xce: UnimplementedInstruction(state); break;
        case 0xcf: UnimplementedInstruction(state); break;
        case 0xd0: UnimplementedInstruction(state); break;
        case 0xd1: UnimplementedInstruction(state); break;
        case 0xd2: UnimplementedInstruction(state); break;
        case 0xd3: UnimplementedInstruction(state); break;
        case 0xd4: UnimplementedInstruction(state); break;
        case 0xd5: UnimplementedInstruction(state); break;
        case 0xd6: UnimplementedInstruction(state); break;
        case 0xd7: UnimplementedInstruction(state); break;
        case 0xd8: UnimplementedInstruction(state); break;
        case 0xd9: UnimplementedInstruction(state); break;
        case 0xda: UnimplementedInstruction(state); break;
        case 0xdb: UnimplementedInstruction(state); break;
        case 0xdc: UnimplementedInstruction(state); break;
        case 0xdd: UnimplementedInstruction(state); break;
        case 0xde: UnimplementedInstruction(state); break;
        case 0xdf: UnimplementedInstruction(state); break;
        case 0xe0: UnimplementedInstruction(state); break;
        case 0xe1: UnimplementedInstruction(state); break;
        case 0xe2: UnimplementedInstruction(state); break;
        case 0xe3: UnimplementedInstruction(state); break;
        case 0xe4: UnimplementedInstruction(state); break;
        case 0xe5: UnimplementedInstruction(state); break;
        case 0xe6: UnimplementedInstruction(state); break;
        case 0xe7: UnimplementedInstruction(state); break;
        case 0xe8: UnimplementedInstruction(state); break;
        case 0xe9: UnimplementedInstruction(state); break;
        case 0xea: UnimplementedInstruction(state); break;
        case 0xeb: UnimplementedInstruction(state); break;
        case 0xec: UnimplementedInstruction(state); break;
        case 0xed: UnimplementedInstruction(state); break;
        case 0xee: UnimplementedInstruction(state); break;
        case 0xef: UnimplementedInstruction(state); break;
        case 0xf0: UnimplementedInstruction(state); break;
        case 0xf1: UnimplementedInstruction(state); break;
        case 0xf2: UnimplementedInstruction(state); break;
        case 0xf3: UnimplementedInstruction(state); break;
        case 0xf4: UnimplementedInstruction(state); break;
        case 0xf5: UnimplementedInstruction(state); break;
        case 0xf6: UnimplementedInstruction(state); break;
        case 0xf7: UnimplementedInstruction(state); break;
        case 0xf8: UnimplementedInstruction(state); break;
        case 0xf9: UnimplementedInstruction(state); break;
        case 0xfa: UnimplementedInstruction(state); break;
        case 0xfb: UnimplementedInstruction(state); break;
        case 0xfc: UnimplementedInstruction(state); break;
        case 0xfd: UnimplementedInstruction(state); break;
        case 0xfe: UnimplementedInstruction(state); break;
        case 0xff: UnimplementedInstruction(state); break;
    }

    state->pc += 1;
}