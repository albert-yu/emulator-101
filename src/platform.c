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
#define MICRO_SECS 16000


#define ROWS FRAME_ROWS
#define COLS FRAME_COLS


void render_bitmap(SDL_Renderer *renderer, uint8_t *framebuf) {
    SDL_SetRenderDrawColor(renderer, BLACK_R, BLACK_G, BLACK_B, ALPHA);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, WHITE_R, WHITE_G, WHITE_B, ALPHA);

    uint8_t pixels, mask, value;

    for (int j = 0; j < COLS; j++) {
        for (int i = 0; i < ROWS;) {
            // 8 pixels per byte
            int offset = (j * ROWS + i) / 8;
            pixels = framebuf[offset];

            for (uint8_t b = 0; b < 8; b++) {
                mask = 1 << b;
                value = mask & pixels; 
                if (value) {
                    SDL_RenderDrawPoint(renderer, i, j);
                }
                i++;
            }
        }
    }

    SDL_RenderPresent(renderer);
}


void loop_machine(Machine *machine) {
    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(ROWS, COLS, 0, &window, &renderer);
    // SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    // for (i = 0; i < WINDOW_WIDTH; ++i)
    //     SDL_RenderDrawPoint(renderer, i, i);
    while (1) {
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
            break;
        }
        // update state
        machine_run(machine, MICRO_SECS);

        // get frame buffer
        uint8_t *framebuf = machine_framebuffer(machine);

        // render pixels
        render_bitmap(renderer, framebuf);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
