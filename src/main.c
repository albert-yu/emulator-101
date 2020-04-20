#include <stdio.h>

#include "core.h" 
#include "disassembler.h"

int main(int argc, char **argv) {
    printf("disassembling\n");

    if (argv) {
        disassemble8080file(argv[0]);
    }
    return 0;
}

