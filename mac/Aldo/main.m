//
//  main.m
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 9/23/22.
//

#import <Foundation/Foundation.h>

#include <SDL2/SDL.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[argc+1])
{
    (void)argv;
    @autoreleasepool {
        NSLog(@"Aldo GUI started...");
    }
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failure: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    int status = EXIT_SUCCESS;
    SDL_Window *const window = SDL_CreateWindow("Aldo",
                                                SDL_WINDOWPOS_UNDEFINED,
                                                SDL_WINDOWPOS_UNDEFINED,
                                                800, 600, 0);
    if (!window) {
        fprintf(stderr, "SDL window creation failure: %s\n", SDL_GetError());
        status = EXIT_FAILURE;
        goto exit_sdl;
    }
    SDL_Surface *const surface = SDL_GetWindowSurface(window);
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0xff, 0xff, 0xff));
    SDL_UpdateWindowSurface(window);
    SDL_Event ev;
    bool running = true;
    do {
        SDL_PollEvent(&ev);
        if (ev.type == SDL_QUIT) {
            running = false;
        }
    } while (running);

    SDL_DestroyWindow(window);
exit_sdl:
    SDL_Quit();
    return status;
}
