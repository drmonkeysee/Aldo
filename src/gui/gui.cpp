//
//  gui.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/24/22.
//

#include "gui.h"
#include "gui.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <SDL2/SDL.h>

#include <concepts>
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

template<std::integral T = int>
struct Vec4 final {
    T x, y, z, w;
};
using U8Vec4 = Vec4<Uint8>;

auto sdl_demo(const aldo::aldo_guiopts& options)
{
    SDLRuntime initSdl;

    static constexpr auto winW = 1280, winH = 800;
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window.get(), renderer.get());
    ImGui_ImplSDLRenderer_Init(renderer.get());

    static constexpr SDL_Rect box{
        (winW - 256) / 2,
        (winH - 240) / 2,
        256,
        240,
    };
    SDL_Rect bouncer{(winW - 50) / 2, (winH - 50) / 2, 50, 50};
    SDL_Point velocity{1, 1};
    SDL_Event ev;
    auto running = true, showDemo = false;
    static constexpr U8Vec4 fillColor{0x1e, 0x1e, 0x1e, SDL_ALPHA_OPAQUE};
    do {
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
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

        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Tools")) {
                ImGui::MenuItem("ImGui Demo", nullptr, &showDemo);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (showDemo) {
            ImGui::ShowDemoWindow();
        }
        ImGui::Begin("First Window");
        ImGui::Text("Hello From Aldo+Dear ImGui");
        ImGui::End();

        ImGui::Render();

        SDL_SetRenderDrawColor(renderer.get(), fillColor.x, fillColor.y,
                               fillColor.z, fillColor.w);
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

        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer.get());
    } while (running);

    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    return EXIT_SUCCESS;
}

}

//
// Public Interface
//

int aldo::rungui_with_args(int, char*[], const aldo::aldo_guiopts& options)
{
    return sdl_demo(options);
}

int aldo::aldo_rungui_with_args(int argc, char* argv[],
                                const aldo::aldo_guiopts* options) noexcept
{
    try {
        return rungui_with_args(argc, argv, *options);
    } catch (const std::exception& ex) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Unhandled error in Aldo: %s", ex.what());
    } catch (...) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unknown error in Aldo!");
    }
    return EXIT_FAILURE;
}
