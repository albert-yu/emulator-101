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

    // declare State8080 struct
    State8080 state;
    // 16-bit address has a maximum of
    // 2^15 addressable 8-bit chunks
    size_t max_size = 1 << 15;
    state.memory = (uint8_t*) malloc(max_size * sizeof(*state.memory));

    // get the file size and read it into a memory buffer
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    uint8_t *buffer = state.memory;

    fread(buffer, fsize, 1, f);
    fclose(f);

    while (1)
    {
        emulate_op(&state);
        print_state(&state);
    }

    free(state.memory);

    return 0;
}
