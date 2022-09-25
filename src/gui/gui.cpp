//
//  gui.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/24/22.
//

#include "gui.h"
#include "gui.hpp"

#include <SDL2/SDL.h>

#include <exception>
#include <stdexcept>

#include <cstdlib>

namespace
{

class SDLRuntime final {
public:
    SDLRuntime()
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                            "SDL initialization failure: %s", SDL_GetError());
            throw std::runtime_error{"SDL init failure"};
        }
        SDL_Log("SDL INIT");
    }

    SDLRuntime(const SDLRuntime&) = delete;
    SDLRuntime& operator=(const SDLRuntime&) = delete;

    ~SDLRuntime()
    {
        SDL_Log("SDL QUIT");
        SDL_Quit();
    }
};

auto sdl_demo(const aldo::initopts& opts)
{
    SDLRuntime initSdl;

    const int winw = 800, winh = 600;
    const Uint32 winflags = opts.hi_dpi ? SDL_WINDOW_ALLOW_HIGHDPI : 0;
    SDL_Window* const window = SDL_CreateWindow("Aldo",
                                                SDL_WINDOWPOS_UNDEFINED,
                                                SDL_WINDOWPOS_UNDEFINED,
                                                winw, winh, winflags);
    if (!window) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL window creation failure: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_Renderer* const
    renderer = SDL_CreateRenderer(window, -1,
                                  SDL_RENDERER_ACCELERATED
                                  | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL renderer creation failure: %s", SDL_GetError());
        // TODO: fix with RAII
        SDL_DestroyWindow(window);
        return EXIT_FAILURE;
    }
    SDL_RenderSetScale(renderer, opts.render_scale_factor,
                       opts.render_scale_factor);
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    SDL_Log("Render name: %s (%X)", info.name, info.flags);

    SDL_Event ev;
    bool running = true;
    const SDL_Rect box{(winw - 256) / 2, (winh - 240) / 2, 256, 240};
    SDL_Rect bouncer{(winw - 50) / 2, (winh - 50) / 2, 50, 50};
    SDL_Point velocity{1, 1};
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
    SDL_DestroyWindow(window);
    return EXIT_SUCCESS;
}

}

//
// Public Interface
//

int aldo::run_with_args(int, char*[], const aldo::initopts& opts)
{
    return sdl_demo(opts);
}

int aldo::aldo_run_with_args(int argc, char* argv[],
                             const aldo::initopts *opts) noexcept
{
    try {
        return run_with_args(argc, argv, *opts);
    } catch (const std::exception& ex) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Unhandled error in Aldo: %s", ex.what());
    } catch (...) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unknown error in Aldo!");
    }
    return EXIT_FAILURE;
}
