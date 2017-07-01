#include "stdio.h"

/*
 * Prints out the mnemonic
 * Returns the size of the operation in bytes
 * http://www.emulator101.com/reference/8080-by-opcode.html
 */
int disassemble8080op(unsigned char *codebuffer, int pc)
{
    unsigned char *code = &codebuffer[pc]; // pointer to the pc-th element in the codebuffer array
    int opbytes = 1;
    printf("%04x ", pc); // formatted as hexadecimal integer

    switch(*code)
    {
        case 0x00: printf("NOP"); break;    
        case 0x01: printf("LXI    B,#$%02x%02x", code[2], code[1]); opbytes = 3; break;    
        case 0x02: printf("STAX   B"); break;    
        case 0x03: printf("INX    B"); break;    
        case 0x04: printf("INR    B"); break;    
        case 0x05: printf("DCR    B"); break;    
        case 0x06: printf("MVI    B,#$%02x", code[1]); opbytes = 2; break;    
        case 0x07: printf("RLC"); break;    
        case 0x08: printf("NOP"); break;    
        case 0x09: printf("DAD    B"); break;
        case 0x0a: printf("LDAX   B"); break;
        case 0x0b: printf("DCX    B"); break;
        case 0x0c: printf("INR    C"); break;
        case 0x0d: printf("DCR    C"); break;
        case 0x0e: printf("MVI    C,#$%02x", code[1]); opbytes = 2; break;
        case 0x0f: printf("RRC"); break;
        case 0x10: printf("NOP"); break;
        case 0x11: printf("LXI    D,#$%02x%02x", code[2], code[1]); opbytes = 3; break;
        case 0x12: printf("STAX   D"); break;
        case 0x13: printf("INX    D"); break;
        case 0x14: printf("INR    D"); break;
        case 0x15: printf("DCR    D"); break;
        case 0x16: printf("MVI    D,#$%02x", code[1]); opbytes = 2; break;
        case 0x17: printf("RAL"); break;
        case 0x18: printf("NOP");
        case 0x19: printf("DAD    D"); break;
        case 0x1a: printf("LDAX   D"); break;
        case 0x1b: printf("DCX    D"); break;
        case 0x1c: printf("INR    E"); break;
        case 0x1d: printf("DCR    E"); break;  
         /* ........ */    
        case 0x3e: printf("MVI    A,#0x%02x", code[1]); opbytes = 2; break;    
        /* ........ */    
        case 0xc3: printf("JMP    $%02x%02x",code[2],code[1]); opbytes = 3; break;    
        /* ........ */    

    }

    return opbytes;
}

