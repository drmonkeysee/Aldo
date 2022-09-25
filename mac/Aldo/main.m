//
//  main.m
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 9/23/22.
//

#include <SDL2/SDL.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

int main(int argc, char *argv[argc+1])
{
    (void)argv;
    SDL_Log("Aldo GUI started...");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL initialization failure: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    int status = EXIT_SUCCESS;
    SDL_Window *const window = SDL_CreateWindow("Aldo",
                                                SDL_WINDOWPOS_UNDEFINED,
                                                SDL_WINDOWPOS_UNDEFINED,
                                                800, 600, 0);
    if (!window) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL window creation failure: %s", SDL_GetError());
        status = EXIT_FAILURE;
        goto exit_sdl;
    }

    SDL_Surface *const surface = SDL_GetWindowSurface(window);
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0xff, 0xff, 0xff));
    SDL_UpdateWindowSurface(window);

    SDL_Event ev;
    bool running = true;
    do {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = false;
            }
        }
    } while (running);

    SDL_DestroyWindow(window);
exit_sdl:
    SDL_Quit();
    return status;
}
