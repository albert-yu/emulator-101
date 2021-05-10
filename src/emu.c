#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "machine.h"
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
            state->memory = NULL;
        }
        free(state);
        state = NULL;
    }
}


#define MAX_STEPS 100000

// 16-bit address has a maximum of
// 2^15 addressable 8-bit chunks
#define MAX_MEM (1 << 15)

/*
 * Returns the number of instructions
 * to advance
 */
size_t get_num_instrs(char *input) {
    if (strlen(input) == 1) {
        return 1;
    }
    // If the string starts with an 
    // alphanumeric character or only 
    // contains alphanumeric characters,
    // 0 is returned.
    size_t steps = (size_t) atoi(input); 
    return steps < MAX_STEPS ? steps : MAX_STEPS; 
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
        .ports = {0, 0, 0, 0, 0, 0, 0}
    };

    // get the file size and read it into a memory buffer
    fseek(f, 0L, SEEK_END);
    int fsize = ftell(f);
    fseek(f, 0L, SEEK_SET);

    fread(state.memory, fsize, 1, f);
    fclose(f);

    size_t instr_count = 0;
    char user_in [20];

    size_t instrs_to_advance = 0;
    while (state.pc < fsize) {
        printf("Emulator state:\n");
        print_state(&state);
        printf("Instructions executed: %zu\n", instr_count);

        if (instrs_to_advance == 0) {
            printf(
                "Press enter to advance one instruction, or " 
                "enter number of instructions to advance "
                "and then press enter: "); 
            fgets(user_in, 20, stdin);
            instrs_to_advance = get_num_instrs(user_in);
            if (instrs_to_advance == 0) {
                continue;
            }
        }
        printf("\n\n");
        // emulate_op(&state, &io);
        machine_step(&machine);
        instr_count++;
        instrs_to_advance--;
    }
    printf("LOOP EXITED.\n");
    print_state(&state);
    printf("fsize: 0x%x\n", fsize);

    free(state.memory);

    return 0;
}
