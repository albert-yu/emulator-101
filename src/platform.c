#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <string.h>

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
#define MICRO_SECS (1e6 / 60) 


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


#define MAX_STEPS 1e7

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


char control_map(SDL_Keycode keycode) {
    char result = 0;
    switch (keycode) {
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
            result = P1_START;
            break;
        case SDLK_LEFT:
            result = P1_JOY_LEFT;
            break;
        case SDLK_RIGHT:
            result = P1_JOY_RIGHT;
            break;
        case SDLK_SPACE:
            result = P1_FIRE;
            break;
        case SDLK_c:
            result = INSERT_COIN;
            break;
    }
    return result;
}


void handle_input(SDL_Event *event, Machine *machine) {
    switch (event->type) {
        case SDL_KEYDOWN:
            machine_keydown(machine, control_map(event->key.keysym.sym));
            break;
        case SDL_KEYUP:
            machine_keyup(machine, control_map(event->key.keysym.sym));
            break;
    }
}


void platform_run(Machine *machine) {
    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;
    uint8_t *framebuf;
    int pending = 0;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(COLS, ROWS, 0, &window, &renderer);
    while (1) {
        pending = SDL_PollEvent(&event);
        if (pending && event.type == SDL_QUIT) {
            break;
        }
        // read input
        handle_input(&event, machine);

        // update state
        machine_run(machine, MICRO_SECS);

        // get frame buffer
        framebuf = machine_framebuffer(machine);

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
            printf("Cycles: %zu\n", machine->cycles);
            printf("Instructions executed: %zu\n", instr_count);

            // render pixels
            uint8_t *framebuf = machine_framebuffer(machine);
            render_bitmap_upright(renderer, framebuf);

            printf(
                "Press enter to advance one instruction, or " 
                "enter number of instructions to advance "
                "and then press enter (press 'q' to quit): "); 
            fgets(user_in, 20, stdin);
            if (strncmp(user_in, "q", 1) == 0) {
                break;
            }
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
