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

#include "imgui.h"
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

constexpr auto shift_pressed(const SDL_Event& ev) noexcept
{
    return ev.key.keysym.mod & KMOD_SHIFT;
}

constexpr auto is_menu_command(const SDL_Event& ev) noexcept
{
    return (ev.key.keysym.mod & KMOD_GUI) && !ev.key.repeat;
}

auto is_free_key(const SDL_Event& ev, bool allowRepeat = false) noexcept
{
    return (allowRepeat || !ev.key.repeat)
            && !ImGui::GetIO().WantCaptureKeyboard;
}

constexpr auto cyclerate_adjust(const SDL_Event& ev) noexcept
{
    return shift_pressed(ev) ? 10 : 1;
}

constexpr auto mode_change(const SDL_Event& ev) noexcept
{
    return shift_pressed(ev) ? -1 : 1;
}

auto handle_keydown(const SDL_Event& ev, const aldo::Emulator& emu,
                    aldo::viewstate& vs)
{
    const auto& lines = emu.snapshot().lines;
    switch (ev.key.keysym.sym) {
    case SDLK_SPACE:
        if (is_free_key(ev)) {
            vs.commands.emplace(aldo::Command::halt, lines.ready);
        }
        break;
    case SDLK_EQUALS:
        if (is_free_key(ev, true)) {
            vs.clock.adjustCycleRate(cyclerate_adjust(ev));
        }
        break;
    case SDLK_MINUS:
        if (is_free_key(ev, true)) {
            vs.clock.adjustCycleRate(-cyclerate_adjust(ev));
        }
        break;
    case SDLK_b:
        if (is_menu_command(ev)) {
            if (ev.key.keysym.mod & KMOD_ALT) {
                if (emu.debugger().isActive()) {
                    vs.commands.emplace(aldo::Command::breakpointsExport);
                }
            } else {
                vs.commands.emplace(aldo::Command::breakpointsOpen);
            }
        }
        break;
    case SDLK_d:
        if (is_menu_command(ev)) {
            vs.showDemo = !vs.showDemo;
        }
        break;
    case SDLK_i:
        if (is_free_key(ev)) {
            vs.addInterruptCommand(CSGI_IRQ, lines.irq);
        }
        break;
    case SDLK_m:
        if (is_free_key(ev)) {
            const auto mode = emu.runMode() + mode_change(ev);
            vs.commands.emplace(aldo::Command::mode,
                                static_cast<csig_excmode>(mode));
        }
        break;
    case SDLK_n:
        if (is_free_key(ev)) {
            vs.addInterruptCommand(CSGI_NMI, lines.nmi);
        }
        break;
    case SDLK_o:
        if (is_menu_command(ev)) {
            vs.commands.emplace(aldo::Command::openROM);
        }
        break;
    case SDLK_s:
        if (is_free_key(ev)) {
            vs.addInterruptCommand(CSGI_RES, lines.reset);
        }
        break;
    }
}

auto process_command(const aldo::command_state& cs, aldo::Emulator& emu,
                     aldo::viewstate& s, const gui_platform& p)
{
    auto& debugger = emu.debugger();
    auto breakpoints = debugger.breakpoints();
    switch (cs.cmd) {
    case aldo::Command::breakpointAdd:
        breakpoints.append(std::get<haltexpr>(cs.value));
        break;
    case aldo::Command::breakpointRemove:
        breakpoints.remove(std::get<aldo::et::diff>(cs.value));
        break;
    case aldo::Command::breakpointsClear:
        breakpoints.clear();
        break;
    case aldo::Command::breakpointsExport:
        aldo::modal::exportBreakpoints(emu, p);
        break;
    case aldo::Command::breakpointsOpen:
        aldo::modal::loadBreakpoints(emu, p);
        break;
    case aldo::Command::breakpointToggle:
        breakpoints.toggleEnabled(std::get<aldo::et::diff>(cs.value));
        break;
    case aldo::Command::halt:
        if (std::get<bool>(cs.value)) {
            emu.halt();
        } else {
            emu.ready();
        }
        break;
    case aldo::Command::interrupt:
        {
            const auto [signal, active] =
                std::get<aldo::command_state::interrupt>(cs.value);
            emu.interrupt(signal, active);
        }
        break;
    case aldo::Command::launchStudio:
        p.launch_studio();
        break;
    case aldo::Command::mode:
        emu.runMode(std::get<csig_excmode>(cs.value));
        break;
    case aldo::Command::openROM:
        aldo::modal::loadROM(emu, p);
        break;
    case aldo::Command::resetVectorClear:
        debugger.vectorClear();
        break;
    case aldo::Command::resetVectorOverride:
        debugger.vectorOverride(std::get<int>(cs.value));
        break;
    case aldo::Command::quit:
        s.running = false;
        break;
    default:
        throw std::domain_error{invalid_command(cs.cmd)};
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
            vs.commands.emplace(aldo::Command::quit);
            break;
        }
    }
    while (!vs.commands.empty()) {
        const auto& cs = vs.commands.front();
        process_command(cs, emu, vs, p);
        vs.commands.pop();
    }
}
