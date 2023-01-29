//
//  input.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/23.
//

#include "input.hpp"

#include "emu.hpp"
#include "guiplatform.h"
#include "modal.hpp"
#include "viewstate.hpp"

#include "imgui_impl_sdl.h"
#include <SDL2/SDL.h>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

namespace
{

auto invalid_command(aldo::Command c)
{
    std::string s = "Invalid gui event command (";
    s += std::to_string(static_cast<std::underlying_type_t<aldo::Command>>(c));
    s += ')';
    return s;
}

auto is_menu_shortcut(const SDL_Event& ev) noexcept
{
    return (ev.key.keysym.mod & KMOD_GUI) && !ev.key.repeat;
}

auto handle_keydown(const SDL_Event& ev, const aldo::Emulator& emu,
                    aldo::viewstate& vs)
{
    switch (ev.key.keysym.sym) {
    case SDLK_b:
        if (is_menu_shortcut(ev)) {
            if (ev.key.keysym.mod & KMOD_ALT) {
                if (emu.debugger().isActive()) {
                    vs.events.emplace(aldo::Command::breakpointsExport);
                }
            } else {
                vs.events.emplace(aldo::Command::breakpointsOpen);
            }
        }
        break;
    case SDLK_d:
        if (is_menu_shortcut(ev)) {
            vs.showDemo = !vs.showDemo;
        }
        break;
    case SDLK_o:
        if (is_menu_shortcut(ev)) {
            vs.events.emplace(aldo::Command::openROM);
        }
        break;
    }
}

auto process_event(const aldo::event& ev, aldo::Emulator& emu,
                   aldo::viewstate& s, const gui_platform& p)
{
    auto& debugger = emu.debugger();
    switch (ev.cmd) {
    case aldo::Command::breakpointAdd:
        debugger.addBreakpoint(std::get<haltexpr>(ev.value));
        break;
    case aldo::Command::breakpointRemove:
        debugger.removeBreakpoint(std::get<aldo::et::diff>(ev.value));
        break;
    case aldo::Command::breakpointsClear:
        debugger.clearBreakpoints();
        break;
    case aldo::Command::breakpointsExport:
        aldo::modal::exportBreakpoints(emu, p);
        break;
    case aldo::Command::breakpointsOpen:
        aldo::modal::loadBreakpoints(emu, p);
        break;
    case aldo::Command::breakpointToggle:
        debugger.toggleBreakpointEnabled(std::get<aldo::et::diff>(ev.value));
        break;
    case aldo::Command::halt:
        if (std::get<bool>(ev.value)) {
            emu.halt();
        } else {
            emu.ready();
        }
        break;
    case aldo::Command::interrupt:
        {
            const auto [signal, active] =
                std::get<aldo::event::interrupt>(ev.value);
            emu.interrupt(signal, active);
        }
        break;
    case aldo::Command::launchStudio:
        p.launch_studio();
        break;
    case aldo::Command::mode:
        emu.runMode(std::get<csig_excmode>(ev.value));
        break;
    case aldo::Command::openROM:
        aldo::modal::loadROM(emu, p);
        break;
    case aldo::Command::overrideReset:
        debugger.vectorOverride(std::get<int>(ev.value));
        break;
    case aldo::Command::quit:
        s.running = false;
        break;
    default:
        throw std::domain_error{invalid_command(ev.cmd)};
    }
}

}

void aldo::input::handle(aldo::Emulator& emu, aldo::viewstate& vs,
                         const gui_platform& p)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        ImGui_ImplSDL2_ProcessEvent(&ev);
        switch (ev.type) {
        case SDL_KEYDOWN:
            handle_keydown(ev, emu, vs);
            break;
        case SDL_QUIT:
            vs.events.emplace(aldo::Command::quit);
            break;
        }
    }
    while (!vs.events.empty()) {
        const auto& ev = vs.events.front();
        process_event(ev, emu, vs, p);
        vs.events.pop();
    }
}
