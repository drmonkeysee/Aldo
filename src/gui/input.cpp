//
//  input.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/28/22.
//

#include "input.hpp"

#include "viewstate.hpp"

#include "imgui_impl_sdl.h"
#include <SDL2/SDL.h>

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
            const auto [signal, active]
                = std::get<aldo::interrupt_event>(ev.value);
            if (active) {
                nes_interrupt(console, signal);
            } else {
                nes_clear(console, signal);
            }
        }
        break;
    default:
        throw std::domain_error{invalid_command(ev.cmd)};
    }
}

}

//
// Public Interface
//

void aldo::handle_input(aldo::viewstate& s, nes* console)
{
    assert(console != nullptr);

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
