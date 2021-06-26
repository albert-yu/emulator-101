#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "emu.h"

int main(int argc, char **argv) {
    int opt;
    EmuMode mode = RUN_MODE;
    while ((opt = getopt(argc, argv, "rsd")) != -1) {
        switch (opt) {
            case 'r': mode = RUN_MODE; break;
            case 's': mode = STEP_MODE; break;
            case 'd': mode = DISASM_MODE; break;
            default:
                fprintf(stderr, "Usage: %s [-rsd] [folder...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    char *folder = argv[optind];
    emu_start(folder, mode);
    return 0;
}

