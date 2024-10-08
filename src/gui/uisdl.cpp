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
#include "ui.h"
#include "view.hpp"
#include "viewstate.hpp"

#include <SDL2/SDL.h>

#include <exception>
#include <cassert>

namespace
{

auto runloop(const gui_platform& p, aldo_debugger* debug, aldo_nes* console)
{
    aldo::Emulator emu{
        aldo::debug_handle{debug}, aldo::console_handle{console}, p,
    };
    aldo::viewstate state;
    aldo::MediaRuntime runtime{{1280, 800}, p};
    aldo::Layout layout{state, emu, runtime};
    SDL_Log("emu: %zu", sizeof emu);
    SDL_Log("state: %zu", sizeof state);
    SDL_Log("runtime: %zu", sizeof runtime);
    SDL_Log("layout: %zu", sizeof layout);
    SDL_Log("total: %zu",
            sizeof emu + sizeof state + sizeof runtime + sizeof layout);
    state.clock.start();
    do {
        auto reset = !emu.snapshot().cpu.lines.ready;
        auto tick = state.clock.startTick(reset);
        aldo::input::handle(emu, state, runtime);
        if (state.running) {
            emu.update(state);
            layout.render();
        }
    } while (state.running);
}

}

//
// MARK: - Public Interface
//

int aldo::ui_sdl_runloop(const gui_platform* platform, aldo_debugger* debug,
                         aldo_nes* console) noexcept
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
    auto err = aldo::MediaRuntime::initStatus();
    if (err < 0) return err;
    return ALDO_UI_ERR_EXCEPTION;
}
