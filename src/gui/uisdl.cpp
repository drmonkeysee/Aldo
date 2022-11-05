//
//  uisdl.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#include "mediaruntime.hpp"
#include "ui.h"
#include "uisdl.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <SDL2/SDL.h>

#include <exception>
#include <cassert>

namespace
{

struct viewstate {
    bool running, showDemo;
};

struct bouncer {
    SDL_Point bounds, pos, velocity;
    int halfdim;
};

auto start_renderframe() noexcept
{
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

auto render_bouncer(const bouncer& bouncer,
                    const aldo::MediaRuntime& runtime) noexcept
{
    const auto ren = runtime.renderer();
    const auto tex = runtime.bouncerTexture();
    SDL_SetRenderTarget(ren, tex);
    SDL_SetRenderDrawColor(ren, 0x0, 0xff, 0xff,SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);

    SDL_SetRenderDrawColor(ren, 0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(ren, 30, 7, 50, 200);

    SDL_SetRenderDrawColor(ren, 0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);
    const auto fulldim = bouncer.halfdim * 2;
    const SDL_Rect pos{
        bouncer.pos.x - bouncer.halfdim,
        bouncer.pos.y - bouncer.halfdim,
        fulldim,
        fulldim,
    };
    SDL_RenderFillRect(ren, &pos);
    SDL_SetRenderTarget(ren, nullptr);

    ImGui::Begin("Bouncer");
    const ImVec2 sz{(float)bouncer.bounds.x, (float)bouncer.bounds.y};
    ImGui::Image(tex, sz);
    ImGui::End();
}

auto end_renderframe(SDL_Renderer* ren)
{
    ImGui::Render();
    SDL_SetRenderDrawColor(ren, 0x1e, 0x1e, 0x1e, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(ren);
}

//
// UI Loop Implementation
//

auto handle_input(viewstate& s) noexcept
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        ImGui_ImplSDL2_ProcessEvent(&ev);
        if (ev.type == SDL_QUIT) {
            s.running = false;
        }
    }
}

auto update_stuff(bouncer& bouncer) noexcept
{
    if (bouncer.pos.x - bouncer.halfdim < 0
        || bouncer.pos.x + bouncer.halfdim > bouncer.bounds.x) {
        bouncer.velocity.x *= -1;
    }
    if (bouncer.pos.y - bouncer.halfdim < 0
        || bouncer.pos.y + bouncer.halfdim > bouncer.bounds.y) {
        bouncer.velocity.y *= -1;
    }
    bouncer.pos.x += bouncer.velocity.x;
    bouncer.pos.y += bouncer.velocity.y;
}

auto render_ui(viewstate& s, const bouncer& bouncer,
               const aldo::MediaRuntime& runtime) noexcept
{
    start_renderframe();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Aldo")) {
            ImGui::MenuItem("About");
            ImGui::MenuItem("ImGui Demo", nullptr, &s.showDemo);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Open");
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    render_bouncer(bouncer, runtime);

    if (s.showDemo) {
        ImGui::ShowDemoWindow();
    }

    end_renderframe(runtime.renderer());
}

auto gui_run(const gui_platform& platform)
{
    bouncer bouncer{{256, 240}, {256 / 2, 240 / 2}, {1, 1}, 25};
    aldo::MediaRuntime runtime{{1280, 800}, bouncer.bounds, platform};
    viewstate state{true, false};
    do {
        handle_input(state);
        if (state.running) {
            update_stuff(bouncer);
            render_ui(state, bouncer, runtime);
        }
    } while (state.running);
}

}

//
// Public Interface
//

int aldo::ui_sdl_runloop(const struct gui_platform* platform) noexcept
{
    assert(platform != nullptr);

    try {
        gui_run(*platform);
        return 0;
    } catch (const std::exception& ex) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "GUI runtime exception: %s", ex.what());
    } catch (...) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Unknown GUI error!");
    }
    const auto err = aldo::MediaRuntime::initStatus();
    if (err < 0) return err;
    return UI_ERR_UNKNOWN;
}
