//
//  uisdl.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#include "uisdl.hpp"

#include "ctrlsignal.h"
#include "input.hpp"
#include "mediaruntime.hpp"
#include "render.hpp"
#include "ui.h"
#include "viewstate.hpp"

#include <SDL2/SDL.h>

#include <exception>
#include <cassert>

namespace
{

auto update_bouncer(aldo::viewstate& s, console_state* snapshot)
{
    if (!snapshot->lines.ready) return;

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

//
// UI Loop Implementation
//

auto emu_update(nes* console, console_state* snapshot,
                aldo::viewstate& s) noexcept
{
    nes_cycle(console, &s.clock.cyclock);
    nes_snapshot(console, snapshot);
    update_bouncer(s, snapshot);
    s.clock.markDtUpdate();
}

auto render_ui(aldo::viewstate& s, const aldo::MediaRuntime& runtime,
               const console_state& snapshot) noexcept
{
    const aldo::RenderFrame frame{runtime};
    frame.render(s, snapshot);
}

auto runloop(const gui_platform& platform, nes* console,
             console_state* snapshot)
{
    aldo::viewstate state;
    const aldo::MediaRuntime runtime{
        {1280, 800}, state.bouncer.bounds, platform,
    };
    state.clock.start();
    do {
        state.clock.tickStart(*snapshot);
        aldo::handle_input(state, console, platform);
        if (state.running) {
            emu_update(console, snapshot, state);
            render_ui(state, runtime, *snapshot);
        }
        state.clock.tickEnd();
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
