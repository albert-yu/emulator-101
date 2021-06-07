#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "emu.h"

int main(int argc, char **argv) {
    int opt;
    enum { RUN_MODE, STEP_MODE, DISASM_MODE } mode = RUN_MODE;
    // if (argv) {
    //     load_and_run(argv[1]);
    // }
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

    switch (mode) {
        case RUN_MODE:
            load_and_run(folder);
            break;
        case STEP_MODE:
        case DISASM_MODE:
            break;
    }
    return 0;
}

