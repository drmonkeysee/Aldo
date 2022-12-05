//
//  uisdl.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#include "uisdl.hpp"

#include "ctrlsignal.h"
#include "emulator.hpp"
#include "mediaruntime.hpp"
#include "render.hpp"
#include "snapshot.h"
#include "ui.h"
#include "viewstate.hpp"

#include <SDL2/SDL.h>

#include <exception>
#include <cassert>

namespace
{

auto update_bouncer(aldo::viewstate& s, const console_state& snapshot) noexcept
{
    if (!snapshot.lines.ready) return;

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

auto emu_update(aldo::EmuController& controller, aldo::viewstate& s) noexcept
{
    controller.update(s);
    update_bouncer(s, controller.snapshot());
    s.clock.markDtUpdate();
}

auto render_ui(aldo::viewstate& s, const aldo::EmuController& c,
               const aldo::MediaRuntime& r)
{
    const aldo::RenderFrame frame{r};
    frame.render(s, c);
}

auto runloop(const gui_platform& platform, debugctx* debug, nes* console)
{
    aldo::EmuController controller{
        aldo::debug_handle{debug}, aldo::console_handle{console},
    };
    aldo::viewstate state;
    const aldo::MediaRuntime runtime{
        {1280, 800}, state.bouncer.bounds, platform,
    };
    state.clock.start();
    do {
        state.clock.tickStart(controller.snapshot());
        controller.handleInput(state, platform);
        if (state.running) {
            emu_update(controller, state);
            render_ui(state, controller, runtime);
        }
        state.clock.tickEnd();
    } while (state.running);
}

}

//
// Public Interface
//

int aldo::ui_sdl_runloop(const gui_platform* platform, debugctx* debug,
                         nes* console) noexcept
{
    assert(platform != nullptr);
    assert(debug != nullptr);
    assert(console != nullptr);

    try {
        runloop(*platform, debug, console);
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
