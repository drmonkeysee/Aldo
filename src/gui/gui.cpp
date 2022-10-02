//
//  gui.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/24/22.
//

#include "gui.h"
#include "gui.hpp"
#include "mediaruntime.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <SDL2/SDL.h>

#include <concepts>
#include <exception>

#include <cassert>
#include <cstdlib>

namespace
{

template<std::integral T = int>
struct Vec4 final {
    T x, y, z, w;
};
using U8Vec4 = Vec4<Uint8>;

auto sdl_demo(const aldo::aldo_guiopts& options)
{
    static constexpr SDL_Point windowSize{1280, 800};
    aldo::MediaRuntime runtime{options, windowSize};

    ImGui::StyleColorsDark();

    static constexpr SDL_Point boxSize{256, 240};
    static constexpr SDL_Rect box{
        (windowSize.x - boxSize.x) / 2,
        (windowSize.y - boxSize.y) / 2,
        boxSize.x,
        boxSize.y,
    };
    static constexpr auto bounceDim = 50;
    SDL_Rect bouncer{
        (windowSize.x - bounceDim) / 2,
        (windowSize.y - bounceDim) / 2,
        bounceDim,
        bounceDim,
    };
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

        SDL_SetRenderDrawColor(runtime.renderer(), fillColor.x, fillColor.y,
                               fillColor.z, fillColor.w);
        SDL_RenderClear(runtime.renderer());

        SDL_SetRenderDrawColor(runtime.renderer(),
                               0x0, 0xff, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(runtime.renderer(), &box);

        SDL_SetRenderDrawColor(runtime.renderer(),
                               0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(runtime.renderer(), &bouncer);

        SDL_SetRenderDrawColor(runtime.renderer(),
                               0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawLine(runtime.renderer(), 100, 80, 250, 500);

        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(runtime.renderer());
    } while (running);

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
    assert(options != nullptr);

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
