//
//  uisdl.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#include "mediaruntime.hpp"
#include "ui.h"
#include "uisdl.hpp"
#include "viewstate.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <SDL2/SDL.h>

#include <exception>
#include <cassert>

namespace
{

auto start_renderframe() noexcept
{
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

auto render_bouncer(const aldo::viewstate& s,
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
    const auto fulldim = s.bouncer.halfdim * 2;
    const SDL_Rect pos{
        s.bouncer.pos.x - s.bouncer.halfdim,
        s.bouncer.pos.y - s.bouncer.halfdim,
        fulldim,
        fulldim,
    };
    SDL_RenderFillRect(ren, &pos);
    SDL_SetRenderTarget(ren, nullptr);

    ImGui::Begin("Bouncer");
    const ImVec2 sz{(float)s.bouncer.bounds.x, (float)s.bouncer.bounds.y};
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

auto handle_input(aldo::viewstate& s) noexcept
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        ImGui_ImplSDL2_ProcessEvent(&ev);
        if (ev.type == SDL_QUIT) {
            s.running = false;
        }
    }
}

auto update_stuff(aldo::viewstate& s) noexcept
{
    if (s.bouncer.pos.x - s.bouncer.halfdim < 0
        || s.bouncer.pos.x + s.bouncer.halfdim > s.bouncer.bounds.x) {
        s.bouncer.velocity.x *= -1;
    }
    if (s.bouncer.pos.y - s.bouncer.halfdim < 0
        || s.bouncer.pos.y + s.bouncer.halfdim > s.bouncer.bounds.y) {
        s.bouncer.velocity.y *= -1;
    }
    s.bouncer.pos.x += s.bouncer.velocity.x;
    s.bouncer.pos.y += s.bouncer.velocity.y;
}

auto render_ui(aldo::viewstate& s, const aldo::MediaRuntime& runtime) noexcept
{
    start_renderframe();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Aldo")) {
            ImGui::MenuItem("About");
            ImGui::MenuItem("ImGui Demo", nullptr, &s.showDemo);
            ImGui::MenuItem("Quit");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Open");
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    render_bouncer(s, runtime);

    if (s.showDemo) {
        ImGui::ShowDemoWindow();
    }

    end_renderframe(runtime.renderer());
}

auto gui_run(const gui_platform& platform)
{
    aldo::viewstate state{
        {{256, 240}, {256 / 2, 240 / 2}, {1, 1}, 25},
        true,
        false,
    };
    aldo::MediaRuntime runtime{{1280, 800}, state.bouncer.bounds, platform};
    do {
        handle_input(state);
        if (state.running) {
            update_stuff(state);
            render_ui(state, runtime);
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
