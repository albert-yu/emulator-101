#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

/*
 * Disassembles a single op and prints it to stdout
 */
int disassemble8080op(unsigned char *codebuffer, int pc);

#endif
