#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "machine.h"
#include "emu.h"
#include "platform.h"

State8080* state_alloc(size_t mem_size) {
    State8080 *state = malloc(sizeof(*state));
    state->memory = (uint8_t*) calloc(mem_size, sizeof(uint8_t));
    return state;
}

void state_free(State8080 *state) {
    if (state) {
        if (state->memory) {
            free(state->memory);
            state->memory = NULL;
        }
        free(state);
        state = NULL;
    }
}


// #define MAX_STEPS 100000

// 16-bit address has a maximum of
// 2^15 addressable 8-bit chunks
#define MAX_MEM (1 << 15)


#define H_START 0x0000
#define G_START 0x0800
#define F_START 0x1000
#define E_START 0x1800

#define CHUNK_SIZE (G_START - H_START)


void load_invaders_chunk(char *invaders_folder, char chunk, uint8_t *memory) {
    // make full path
    size_t folder_len = strlen(invaders_folder);
    char *folder_path = calloc(folder_len + 16, sizeof(*folder_path));
    sprintf(folder_path, "%s/invaders.%c", invaders_folder, chunk);

    // load the file
    FILE *f = fopen(folder_path, "rb");
    if (f == NULL) {
        printf("Error: couldn't open %s\n", folder_path);
        exit(1);
    }

    // get the file size and read it into a memory buffer
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    if (fsize != CHUNK_SIZE) {
        printf("WARNING: unexpected chunk size %d, expected %d\n", fsize, CHUNK_SIZE);
    }
    fseek(f, 0L, SEEK_SET);

    int offset = 0;
    switch (chunk) {
        case 'h':
            offset = H_START;
            break;
        case 'g':
            offset = G_START;
            break;
        case 'f':
            offset = F_START;
            break;
        case 'e':
            offset = E_START;
            break;
    }
    fread(memory + offset, fsize, 1, f);
    fclose(f);
}


void load_invaders(char *invaders_folder, uint8_t *memory) {
    load_invaders_chunk(invaders_folder, 'h', memory);
    load_invaders_chunk(invaders_folder, 'g', memory);
    load_invaders_chunk(invaders_folder, 'f', memory);
    load_invaders_chunk(invaders_folder, 'e', memory);
}


int emu_start(char *folder, EmuMode mode) {
    // declare ConditionCodes struct
    ConditionCodes cc;
    cc = (ConditionCodes) {
        .z = 0,
        .s = 0,
        .p = 0,
        .cy = 0,
        .ac = 0
    };

    // declare State8080 struct
    State8080 state;
    state = (State8080) {
        .a = 0,
        .b = 0,
        .c = 0,
        .d = 0,
        .e = 0,
        .h = 0,
        .l = 0,
        .sp = 0,
        .pc = 0,
        .int_enable = 0,
        .int_delay = 0,
        .int_pending = 0,
        .int_type = 0,
        .cycles = 0,
        .memory = (uint8_t*) malloc(MAX_MEM * sizeof(*state.memory)),
        .cc = cc
    };

    IO8080 io;
    io = (IO8080) {
        .in = 0,
        .out = 0,
        .port = 0,
        .value = 0
    };

    Machine machine;
    machine = (Machine) {
        .cpu_state = &state,
        .shift_register = 0,
        .io = &io,
        .ports = {0, 0, 0, 0, 0, 0, 0},
        .int_type = 1,
    };

    load_invaders(folder, state.memory);

    switch (mode) {
        case RUN_MODE:
            platform_run(&machine);
            break;
        case STEP_MODE:
            platform_step(&machine);
            break;
        case DISASM_MODE:
            fprintf(stderr, "Disassembler not implemented yet\n");
            break;
    }

    free(state.memory);

    return 0;
}
