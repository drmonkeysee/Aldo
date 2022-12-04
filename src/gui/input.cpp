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

auto is_guikey_shortcut(SDL_Event* ev) noexcept
{
    return ev->key.keysym.mod == KMOD_LGUI || ev->key.keysym.mod == KMOD_RGUI;
}

auto open_cart_file(const gui_platform& p) noexcept
{
    char filepath[1024];
    size_t len = 0;
    const auto ok = p.open_file(sizeof filepath, filepath, &len);
    if (ok) {
        SDL_Log("File selected: %s", filepath);
    }
    if (!ok && len > 0) {
        SDL_Log("Filepath needed %zu bytes", len);
        assert(((void)"Cart filepath buffer too small", false));
    }
}

auto process_event(const aldo::event& ev, aldo::viewstate& s, nes* console,
                   const gui_platform& p)
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
    case aldo::Command::openFile:
        open_cart_file(p);
        break;
    case aldo::Command::quit:
        s.running = false;
        break;
    case aldo::Command::interrupt:
        {
            const auto [signal, active] =
                std::get<aldo::interrupt_event>(ev.value);
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

void aldo::handle_input(aldo::viewstate& s, nes* console,
                        const gui_platform& p)
{
    assert(console != nullptr);

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        ImGui_ImplSDL2_ProcessEvent(&ev);
        switch (ev.type) {
        case SDL_KEYDOWN:
            if (is_guikey_shortcut(&ev) && !ev.key.repeat
                && ev.key.keysym.sym == SDLK_o) {
                s.queueOpenFile();
            }
            break;
        case SDL_QUIT:
            s.events.emplace(aldo::Command::quit);
            break;
        }
    }
    while (!s.events.empty()) {
        const auto& ev = s.events.front();
        process_event(ev, s, console, p);
        s.events.pop();
    }
}
