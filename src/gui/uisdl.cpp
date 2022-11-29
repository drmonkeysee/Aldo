//
//  uisdl.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#include "uisdl.hpp"

#include "ctrlsignal.h"
#include "mediaruntime.hpp"
#include "render.hpp"
#include "ui.h"
#include "viewstate.hpp"

#include "imgui_impl_sdl.h"
#include <SDL2/SDL.h>

#include <exception>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <cassert>

namespace
{

auto invalid_command(aldo::Command c)
{
    std::stringstream s;
    s
        << "Invalid gui event command ("
        << static_cast<std::underlying_type_t<aldo::Command>>(c)
        << ')';
    return s.str();
}

auto process_event(const aldo::event& ev, aldo::viewstate& state, nes* console)
{
    switch (ev.cmd) {
    case aldo::Command::halt:
        if (std::get<bool>(ev.value)) {
            nes_halt(console);
        } else {
            nes_ready(console);
        }
        break;
    case aldo::Command::mode:
        nes_mode(console, std::get<csig_excmode>(ev.value));
        break;
    case aldo::Command::quit:
        state.running = false;
        break;
    case aldo::Command::interrupt:
        {
            const auto& val = std::get<aldo::interrupt_event>(ev.value);
            if (val.second) {
                nes_interrupt(console, val.first);
            } else {
                nes_clear(console, val.first);
            }
        }
        break;
    default:
        throw std::domain_error{invalid_command(ev.cmd)};
    }
}

//
// UI Loop Implementation
//

auto handle_input(aldo::viewstate& s, nes* console)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        ImGui_ImplSDL2_ProcessEvent(&ev);
        if (ev.type == SDL_QUIT) {
            s.events.emplace(aldo::Command::quit);
        }
    }
    while (!s.events.empty()) {
        const auto& ev = s.events.front();
        process_event(ev, s, console);
        s.events.pop();
    }
}

auto emu_update(nes* console, console_state* snapshot,
                aldo::viewstate& s) noexcept
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
    nes_cycle(console, &s.clock.cyclock);
    nes_snapshot(console, snapshot);
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
        handle_input(state, console);
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
