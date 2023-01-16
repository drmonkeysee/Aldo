//
//  emulator.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/4/22.
//

#include "ctrlsignal.h"
#include "emulator.hpp"
#include "error.hpp"
#include "guiplatform.h"
#include "haltexpr.h"
#include "viewstate.hpp"

#include "imgui_impl_sdl.h"
#include <SDL2/SDL.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <cerrno>
#include <cstdio>

namespace
{

using file_handle = aldo::handle<std::FILE, std::fclose>;

auto invalid_command(aldo::Command c)
{
    std::string s = "Invalid gui event command (";
    s += std::to_string(static_cast<std::underlying_type_t<aldo::Command>>(c));
    s += ')';
    return s;
}

auto is_guikey_mod(const SDL_Event& ev) noexcept
{
    return ev.key.keysym.mod == KMOD_LGUI || ev.key.keysym.mod == KMOD_RGUI;
}

auto handle_keydown(const SDL_Event& ev, aldo::viewstate& state)
{
    switch (ev.key.keysym.sym) {
    case SDLK_d:
        if (is_guikey_mod(ev) && !ev.key.repeat) {
            state.showDemo = !state.showDemo;
        }
        break;
    case SDLK_o:
        if (is_guikey_mod(ev) && !ev.key.repeat) {
            state.events.emplace(aldo::Command::openFile);
        }
        break;
    }
}

}

//
// Public Interface
//

std::string_view aldo::EmuController::cartName() const noexcept
{
    // NOTE: not a ternary because the expression must evaluate to type
    // const std::string& which converts cart_errstr to a dangling temporary.
    if (cartFilestem.empty()) return cart_errstr(CART_ERR_NOCART);
    return cartFilestem.native();
}

std::optional<cartinfo> aldo::EmuController::cartInfo() const
{
    if (!hcart) return {};

    cartinfo info;
    cart_getinfo(cartp(), &info);
    return info;
}

void aldo::EmuController::handleInput(aldo::viewstate& state,
                                      const gui_platform& platform)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        ImGui_ImplSDL2_ProcessEvent(&ev);
        switch (ev.type) {
        case SDL_KEYDOWN:
            handle_keydown(ev, state);
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

void aldo::EmuController::update(aldo::viewstate& state) noexcept
{
    nes_cycle(consolep(), &state.clock.cyclock);
    nes_snapshot(consolep(), snapshotp());
    updateBouncer(state);
}

//
// Private Interface
//

void aldo::EmuController::loadCartFrom(const char* filepath)
{
    cart* c;
    errno = 0;
    file_handle f{std::fopen(filepath, "rb")};
    if (f) {
        const int err = cart_create(&c, f.get());
        if (err < 0) {
            throw aldo::AldoError{"Cart load failure", err, cart_errstr};
        }
    } else {
        throw aldo::AldoError{"Cannot open cart file", filepath, errno};
    }
    nes_powerdown(consolep());
    hcart.reset(c);
    nes_powerup(consolep(), cartp(), false);
    cartFilepath = filepath;
    cartFilestem = cartFilepath.stem();
}

void aldo::EmuController::openCartFile(const gui_platform& p)
{
    // NOTE: halt console to prevent time-jump from modal delay
    // TODO: does this make sense long-term?
    nes_halt(consolep());

    const aldo::platform_buffer filepath{p.open_file(), p.free_buffer};
    if (!filepath) return;

    SDL_Log("File selected: %s", filepath.get());
    try {
        loadCartFrom(filepath.get());
    } catch (const aldo::AldoError& err) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
        p.display_error(err.title(), err.message());
    }
}

void aldo::EmuController::processEvent(const aldo::event& ev,
                                       aldo::viewstate& s,
                                       const gui_platform& p)
{
    switch (ev.cmd) {
    case aldo::Command::breakpointAdd:
        debug_bp_add(debugp(), std::get<haltexpr>(ev.value));
        break;
    case aldo::Command::breakpointsClear:
        debug_bp_clear(debugp());
        break;
    case aldo::Command::breakpointRemove:
        debug_bp_remove(debugp(), std::get<aldo::et::diff>(ev.value));
        break;
    case aldo::Command::breakpointToggle:
        {
            const auto idx = std::get<aldo::et::diff>(ev.value);
            const auto bp = breakpointAt(idx);
            debug_bp_enabled(debugp(), idx, bp && !bp->enabled);
        }
        break;
    case aldo::Command::halt:
        if (std::get<bool>(ev.value)) {
            nes_halt(consolep());
        } else {
            nes_ready(consolep());
        }
        break;
    case aldo::Command::interrupt:
        {
            const auto [signal, active] =
                std::get<aldo::event::interrupt>(ev.value);
            if (active) {
                nes_interrupt(consolep(), signal);
            } else {
                nes_clear(consolep(), signal);
            }
        }
        break;
    case aldo::Command::launchStudio:
        p.launch_studio();
        break;
    case aldo::Command::mode:
        nes_mode(consolep(), std::get<csig_excmode>(ev.value));
        break;
    case aldo::Command::openFile:
        openCartFile(p);
        break;
    case aldo::Command::overrideReset:
        debug_set_resetvector(debugp(), std::get<int>(ev.value));
        break;
    case aldo::Command::quit:
        s.running = false;
        break;
    default:
        throw std::domain_error{invalid_command(ev.cmd)};
    }
}

void aldo::EmuController::updateBouncer(aldo::viewstate& s) const noexcept
{
    if (!snapshot().lines.ready) return;

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
