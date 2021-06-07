#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#include "machine.h"
#include "platform.h"

#define WINDOW_WIDTH 600

#define BLACK_R 0
#define BLACK_G 0
#define BLACK_B 0

#define WHITE_R 255
#define WHITE_G 255
#define WHITE_B 255

#define ALPHA 255
#define MICRO_SECS 10000


#define ROWS FRAME_ROWS
#define COLS FRAME_COLS


void render_bitmap_upright(SDL_Renderer *renderer, uint8_t *framebuf) {
    SDL_SetRenderDrawColor(renderer, BLACK_R, BLACK_G, BLACK_B, ALPHA);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, WHITE_R, WHITE_G, WHITE_B, ALPHA);

    uint8_t pixels_octet, mask, value;

    for (int j = 0; j < COLS; j++) {
        for (int i = 0; i < ROWS;) {
            // 8 pixels per byte
            int offset = (j * ROWS + i) / 8;
            pixels_octet = framebuf[offset];

            for (uint8_t b = 0; b < 8; b++) {
                mask = 1 << b;
                value = mask & pixels_octet;
                if (value) {
                    SDL_RenderDrawPoint(renderer, j, ROWS - i);
                }
                i++;
            }
        }
    }

    SDL_RenderPresent(renderer);
}


#define MAX_STEPS 1e6

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


void platform_run(Machine *machine) {
    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(COLS, ROWS, 0, &window, &renderer);
    while (1) {
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
            break;
        }
        // update state
        machine_run(machine, MICRO_SECS);

        // get frame buffer
        uint8_t *framebuf = machine_framebuffer(machine);

        // render pixels
        render_bitmap_upright(renderer, framebuf);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}


void platform_step(Machine *machine) {
    size_t instr_count = 0;
    char user_in [20];

    size_t instrs_to_advance = 0;
    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(COLS, ROWS, 0, &window, &renderer);
    while (1) {
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
            break;
        }
        // update state
        if (instrs_to_advance == 0) {
            // show state
            printf("Emulator state:\n");
            cpu_print_state(machine->cpu_state);
            printf("Instructions executed: %zu\n", instr_count);

            // render pixels
            uint8_t *framebuf = machine_framebuffer(machine);
            render_bitmap_upright(renderer, framebuf);

            printf(
                "Press enter to advance one instruction, or " 
                "enter number of instructions to advance "
                "and then press enter: "); 
            fgets(user_in, 20, stdin);
            instrs_to_advance = get_num_instrs(user_in);
            if (instrs_to_advance == 0) {
                continue;
            }
            printf("\n\n");
        }

        machine_step(machine);
        instr_count++;
        instrs_to_advance--;

    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
