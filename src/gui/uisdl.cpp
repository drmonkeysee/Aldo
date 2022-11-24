//
//  uisdl.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#include "uisdl.hpp"

#include "event.hpp"
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

auto handle_input(aldo::viewstate& s, nes* console)
{
    aldo::handle_events(s.events, console);

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        ImGui_ImplSDL2_ProcessEvent(&ev);
        if (ev.type == SDL_QUIT) {
            s.running = false;
        }
    }
}

auto update_stuff(aldo::viewstate& s, nes* console,
                  console_state* snapshot) noexcept
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
    nes_snapshot(console, snapshot);
}

auto render_ui(aldo::viewstate& s, const aldo::MediaRuntime& runtime,
               const console_state* snapshot) noexcept
{
    const aldo::RenderFrame frame{runtime};
    frame.render(s, *snapshot);
}

auto runloop(const gui_platform& platform, nes* console,
             console_state* snapshot)
{
    aldo::viewstate state{
        .bouncer{{256, 240}, {256 / 2, 240 / 2}, {1, 1}, 25},
    };
    const aldo::MediaRuntime runtime{
        {1280, 800}, state.bouncer.bounds, platform,
    };
    do {
        handle_input(state, console);
        if (state.running) {
            update_stuff(state, console, snapshot);
            render_ui(state, runtime, snapshot);
        }
    } while (state.running);
}

}

//
// Public Interface
//

int aldo::ui_sdl_runloop(const gui_platform* platform, nes* console,
                         console_state* snapshot) noexcept
{
    assert(platform != nullptr);
    assert(console != nullptr);
    assert(snapshot != nullptr);

    try {
        runloop(*platform, console, snapshot);
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
