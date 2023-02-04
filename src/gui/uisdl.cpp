//
//  uisdl.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#include "uisdl.hpp"

#include "debug.hpp"
#include "emu.hpp"
#include "input.hpp"
#include "mediaruntime.hpp"
#include "render.hpp"
#include "ui.h"
#include "view.hpp"
#include "viewstate.hpp"

#include <SDL2/SDL.h>

#include <exception>
#include <cassert>

namespace
{

//
// UI Loop Implementation
//

auto handle_input(aldo::Emulator& emu, aldo::viewstate& vs,
                  const gui_platform& p)
{
    const auto timer = vs.clock.timeInput();
    aldo::input::handle(emu, vs, p);
}

auto update_emu(aldo::Emulator& emu, aldo::viewstate& vs) noexcept
{
    const auto timer = vs.clock.timeUpdate();
    emu.update(vs);
}

auto render_ui(const aldo::Layout& l, const aldo::MediaRuntime& mr,
               aldo::viewstate& vs)
{
    const aldo::RenderFrame frame{mr, vs.clock.timeRender()};
    l.render();
}

auto runloop(const gui_platform& p, debugctx* debug, nes* console)
{
    aldo::Emulator emu{
        aldo::debug_handle{debug}, aldo::console_handle{console}, p,
    };
    aldo::viewstate state;
    const aldo::MediaRuntime runtime{{1280, 800}, state.bouncer.bounds, p};
    const aldo::Layout layout{state, emu, runtime};
    state.clock.start();
    do {
        const auto reset = !emu.snapshot().lines.ready;
        const auto tick = state.clock.startTick(reset);
        handle_input(emu, state, p);
        if (state.running) {
            update_emu(emu, state);
            render_ui(layout, runtime, state);
        }
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
    return UI_ERR_EXCEPTION;
}
