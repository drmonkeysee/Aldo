//
//  uisdl.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#include "uisdl.hpp"

#include "mediaruntime.hpp"
#include "render.hpp"
#include "ui.h"
#include "viewstate.hpp"

#include "imgui_impl_sdl.h"
#include <SDL2/SDL.h>

#include <exception>
#include <cassert>

namespace
{

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
    const aldo::RenderFrame frame{s, runtime};
    frame.render();
}

auto runloop(const gui_platform& platform)
{
    aldo::viewstate state{
        {{256, 240}, {256 / 2, 240 / 2}, {1, 1}, 25},
    };
    const aldo::MediaRuntime runtime{
        {1280, 800}, state.bouncer.bounds, platform,
    };
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
        runloop(*platform);
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
