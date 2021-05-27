#include <stdio.h>

#include "emu.h"
#include "platform.h"

int main(int argc, char **argv) {
    if (argv) {
        // disassemble8080file(argv[1]);
        try_window();
        load_and_run(argv[1]);
    }
    return 0;
}

