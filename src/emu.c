#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "emu.h"

State8080* state_alloc(size_t mem_size) {
    State8080 *state = malloc(sizeof(*state));
    state->memory = (uint8_t*) calloc(mem_size, sizeof(uint8_t));
    return state;
}

void state_free(State8080 *state) {
    if (state) {
        if (state->memory) {
            free(state->memory);
        }
        state = NULL;
    }
}


int load_and_run(char *filename) {
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
    {
        printf("Error: couldn't open %s\n", filename);
        exit(1);
    }

    // declare ConditionCodes struct
    ConditionCodes cc;
    cc.z = 0;
    cc.s = 0;
    cc.p = 0;
    cc.cy = 0;
    cc.ac = 0;

    // declare State8080 struct
    State8080 state;
    state.a = 0;
    state.b = 0;
    state.c = 0;
    state.d = 0;
    state.e = 0;
    state.h = 0;
    state.l = 0;

    state.sp = 0xffff;
    state.pc = 0;
    state.int_enable = 0;
    // 16-bit address has a maximum of
    // 2^15 addressable 8-bit chunks
    size_t max_size = 1 << 15;
    state.memory = (uint8_t*) malloc(max_size * sizeof(*state.memory));

    state.cc = cc;

    // get the file size and read it into a memory buffer
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    // uint8_t *buffer = state.memory;

    fread(state.memory, fsize, 1, f);
    fclose(f);

    printf("fsize: %d\n", fsize);
    size_t instr_count = 0;

    while (state.pc < fsize)
    {
        printf("Emulator state:\n");
        print_state(&state);
        printf("Instructions executed: %zu\n", instr_count);

        printf("Press enter to continue\n"); 
        getchar();
        emulate_op(&state);

        // // inspect memory location
        // // at 20c0
        // uint16_t mem_loc = 0x20c0;
        // uint8_t val = state.memory[mem_loc];
        // if (val == 0 && instr_count > 40000) {
        //     printf("IS ZERO\n");
        // }
        instr_count++;
    }

    free(state.memory);

    return 0;
}
