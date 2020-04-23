#include <stdio.h>

#include "disassembler.h"
#include "emu.h"

int main(int argc, char **argv) {
    if (argv) {
        disassemble8080file(argv[0]);
        // load_and_run(argv[0]);
    }
    return 0;
}

