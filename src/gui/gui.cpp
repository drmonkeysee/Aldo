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
#include <memory>
#include <stdexcept>
#include <type_traits>

#include <cstdlib>

namespace
{

template<auto d>
using sdl_deleter = std::integral_constant<std::decay_t<decltype(d)>, d>;
using winhandle = std::unique_ptr<SDL_Window, sdl_deleter<SDL_DestroyWindow>>;
using renderhandle =
    std::unique_ptr<SDL_Renderer, sdl_deleter<SDL_DestroyRenderer>>;

class SDLRuntime final {
public:
    SDLRuntime()
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                            "SDL initialization failure: %s", SDL_GetError());
            throw std::runtime_error{"SDL runtime failure"};
        }
    }

    SDLRuntime(const SDLRuntime&) = delete;
    SDLRuntime& operator=(const SDLRuntime&) = delete;

    ~SDLRuntime() { SDL_Quit(); }
};

auto sdl_demo(const aldo::guiopts& options)
{
    SDLRuntime initSdl;

    static constexpr auto winW = 800, winH = 600;
    const Uint32 winFlags = options.hi_dpi ? SDL_WINDOW_ALLOW_HIGHDPI : 0;
    const winhandle window{SDL_CreateWindow("Aldo",
                                            SDL_WINDOWPOS_UNDEFINED,
                                            SDL_WINDOWPOS_UNDEFINED,
                                            winW, winH, winFlags)};
    if (!window) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL window creation failure: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    static constexpr Uint32 renderFlags = SDL_RENDERER_ACCELERATED
                                            | SDL_RENDERER_PRESENTVSYNC;
    const renderhandle renderer{SDL_CreateRenderer(window.get(), -1,
                                                   renderFlags)};
    if (!renderer) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL renderer creation failure: %s", SDL_GetError());
        return EXIT_FAILURE;
    }
    SDL_RenderSetScale(renderer.get(), options.render_scale_factor,
                       options.render_scale_factor);
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer.get(), &info);
    SDL_Log("Render name: %s (%X)", info.name, info.flags);

    static constexpr SDL_Rect box{
        (winW - 256) / 2,
        (winH - 240) / 2,
        256,
        240,
    };
    SDL_Rect bouncer{(winW - 50) / 2, (winH - 50) / 2, 50, 50};
    SDL_Point velocity{1, 1};
    SDL_Event ev;
    auto running = true;
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

        SDL_SetRenderDrawColor(renderer.get(),
                               0xff, 0xff, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer.get());

        SDL_SetRenderDrawColor(renderer.get(),
                               0x0, 0xff, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer.get(), &box);

        SDL_SetRenderDrawColor(renderer.get(),
                               0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer.get(), &bouncer);

        SDL_SetRenderDrawColor(renderer.get(),
                               0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(renderer.get(), 100, 80, 250, 500);

        SDL_RenderPresent(renderer.get());
    } while (running);

    return EXIT_SUCCESS;
}

}

//
// Public Interface
//

int aldo::run_with_args(int, char*[], const aldo::guiopts& options)
{
    return sdl_demo(options);
}

int aldo::aldo_run_with_args(int argc, char* argv[],
                             const aldo::guiopts *options) noexcept
{
    try {
        return run_with_args(argc, argv, *options);
    } catch (const std::exception& ex) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Unhandled error in Aldo: %s", ex.what());
    } catch (...) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unknown error in Aldo!");
    }
    return EXIT_FAILURE;
}
