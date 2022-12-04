//
//  emulator.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/4/22.
//

#include "emulator.hpp"
#include "viewstate.hpp"

#include "imgui_impl_sdl.h"
#include <SDL2/SDL.h>

#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <cassert>
#include <cstddef>

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

}

//
// Public Interface
//

std::string_view aldo::EmuController::cartName() const
{
    if (cartFilepath.empty()) return "NO CART";

    std::string_view v = cartFilepath;
    auto slash = v.rfind('/');
    const auto dot = v.rfind('.');
    if (dot < slash) return v;

    if (slash == std::string_view::npos) {
        slash = 0;
    } else {
        ++slash;    // NOTE: one-past the last slash
    }
    // NOTE: if not found dot will be npos which will work for end-of-string,
    // even with slash subtracted.
    return v.substr(slash, dot - slash);
}

void aldo::EmuController::handleInput(aldo::viewstate& state,
                                      const gui_platform& platform)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        ImGui_ImplSDL2_ProcessEvent(&ev);
        switch (ev.type) {
        case SDL_KEYDOWN:
            if (is_guikey_shortcut(&ev) && !ev.key.repeat
                && ev.key.keysym.sym == SDLK_o) {
                state.events.emplace(aldo::Command::openFile);
            }
            break;
        case SDL_QUIT:
            state.events.emplace(aldo::Command::quit);
            break;
        }
    }
    while (!state.events.empty()) {
        const auto& ev = state.events.front();
        processEvent(ev, state, platform);
        state.events.pop();
    }
}

void aldo::EmuController::update(aldo::viewstate& state,
                                 console_state& snapshot) const noexcept
{
    nes_cycle(hconsole.get(), &state.clock.cyclock);
    nes_snapshot(hconsole.get(), &snapshot);
}

//
// Private Interface
//

void aldo::EmuController::loadCartFrom(std::string_view filepath)
{
    cartFilepath = filepath;
}

void aldo::EmuController::openCartFile(const gui_platform& p)
{
    char filepath[1024];
    std::size_t len;
    // NOTE: halt console before popping modal to prevent time jump on close
    // TODO: does this make sense long-term?
    nes_halt(hconsole.get());
    const auto ok = p.open_file(sizeof filepath, filepath, &len);
    if (ok) {
        SDL_Log("File selected: %s", filepath);
        loadCartFrom(filepath);
    }
    if (!ok && len > 0) {
        SDL_Log("Filepath needed %zu bytes", len);
        assert(((void)"Cart filepath buffer too small", false));
    }
}

void aldo::EmuController::processEvent(const event& ev, viewstate& s,
                                       const gui_platform& p)
{
    switch (ev.cmd) {
    case aldo::Command::halt:
        if (std::get<bool>(ev.value)) {
            nes_halt(hconsole.get());
        } else {
            nes_ready(hconsole.get());
        }
        break;
    case aldo::Command::mode:
        nes_mode(hconsole.get(), std::get<csig_excmode>(ev.value));
        break;
    case aldo::Command::openFile:
        openCartFile(p);
        break;
    case aldo::Command::quit:
        s.running = false;
        break;
    case aldo::Command::interrupt:
        {
            const auto [signal, active] =
                std::get<aldo::interrupt_event>(ev.value);
            if (active) {
                nes_interrupt(hconsole.get(), signal);
            } else {
                nes_clear(hconsole.get(), signal);
            }
        }
        break;
    default:
        throw std::domain_error{invalid_command(ev.cmd)};
    }
}
