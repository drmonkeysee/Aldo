//
//  main.m
//  Aldo-Gui
//
//  Created by Brandon Stansbury on 9/23/22.
//

#import <AppKit/AppKit.h>

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

    const int winw = 800, winh = 600;
    int status = EXIT_SUCCESS;
    SDL_Window *const window = SDL_CreateWindow("Aldo",
                                                SDL_WINDOWPOS_UNDEFINED,
                                                SDL_WINDOWPOS_UNDEFINED,
                                                winw, winh,
                                                SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL window creation failure: %s", SDL_GetError());
        status = EXIT_FAILURE;
        goto exit_sdl;
    }

    SDL_Renderer *const
    renderer = SDL_CreateRenderer(window, -1,
                                  SDL_RENDERER_ACCELERATED
                                  | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL renderer creation failure: %s", SDL_GetError());
        status = EXIT_FAILURE;
        goto exit_window;
    }
    const float scale_factor = NSScreen.mainScreen.backingScaleFactor;
    SDL_RenderSetScale(renderer, scale_factor, scale_factor);

    SDL_Event ev;
    bool running = true;
    const SDL_Rect box = {(winw - 256) / 2, (winh - 240) / 2, 256, 240};
    SDL_Rect bouncer = {(winw - 50) / 2, (winh - 50) / 2, 50, 50};
    SDL_Point velocity = {1, 1};
    do {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = false;
            }
        }

        bouncer.x += velocity.x;
        bouncer.y += velocity.y;
        if (bouncer.x < box.x || (bouncer.x + bouncer.w) > (box.x + box.w)) {
            velocity.x *= -1;
        }
        if (bouncer.y < box.y || (bouncer.y + bouncer.h) > (box.y + box.h)) {
            velocity.y *= -1;
        }

        SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 0x0, 0xff, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &box);

        SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &bouncer);

        SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(renderer, 100, 80, 250, 500);

        SDL_RenderPresent(renderer);
    } while (running);

    SDL_DestroyRenderer(renderer);
exit_window:
    SDL_DestroyWindow(window);
exit_sdl:
    SDL_Quit();
    return status;
}
