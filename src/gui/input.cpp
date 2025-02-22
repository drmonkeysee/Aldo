//
//  input.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/23.
//

#include "input.hpp"

#include "emu.hpp"
#include "mediaruntime.hpp"
#include "modal.hpp"
#include "viewstate.hpp"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include <SDL2/SDL.h>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <cerrno>
#include <cstring>

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

constexpr auto rate_adjust(const SDL_Event& ev) noexcept
{
    return shift_pressed(ev) ? 10 : 1;
}

constexpr auto mode_change(const SDL_Event& ev) noexcept
{
    return shift_pressed(ev) ? -1 : 1;
}

auto breakpoint_add_failed()
{
    const auto* errMsg = std::strerror(errno);
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                 "Add breakpoint error (%d): (%s)", errno, errMsg);
    std::string msg = "Unable to add breakpoint (";
    msg += std::to_string(errno);
    msg += "): ";
    msg += errMsg;
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Add Breakpoint Failure",
                             msg.c_str(), nullptr);
}

auto handle_keydown(const SDL_Event& ev, const aldo::Emulator& emu,
                    aldo::viewstate& vs)
{
    auto& lines = emu.snapshot().cpu.lines;
    switch (ev.key.keysym.sym) {
    case SDLK_SPACE:
        if (is_free_key(ev)) {
            vs.commands.emplace(aldo::Command::ready, !lines.ready);
        }
        break;
    case SDLK_EQUALS:
        if (is_free_key(ev, true)) {
            vs.clock.adjustRate(rate_adjust(ev));
        }
        break;
    case SDLK_MINUS:
        if (is_free_key(ev, true)) {
            vs.clock.adjustRate(-rate_adjust(ev));
        }
        break;
    case SDLK_0:
        if (is_menu_command(ev)) {
            vs.commands.emplace(aldo::Command::zeroRamOnPowerup, !emu.zeroRam);
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
    case SDLK_c:
        if (is_free_key(ev)) {
            vs.clock.toggleScale();
        }
        break;
    case SDLK_d:
        if (is_menu_command(ev)) {
            vs.showDemo = !vs.showDemo;
        }
        break;
    case SDLK_i:
        if (is_free_key(ev)) {
            vs.addProbeCommand(ALDO_INT_IRQ, !emu.probe(ALDO_INT_IRQ));
        }
        break;
    case SDLK_m:
        if (is_free_key(ev)) {
            auto mode = emu.runMode() + mode_change(ev);
            vs.commands.emplace(aldo::Command::mode,
                                static_cast<aldo_execmode>(mode));
        }
        break;
    case SDLK_n:
        if (is_free_key(ev)) {
            vs.addProbeCommand(ALDO_INT_NMI, !emu.probe(ALDO_INT_NMI));
        }
        break;
    case SDLK_o:
        if (is_menu_command(ev)) {
            vs.commands.emplace(aldo::Command::openROM);
        }
        break;
    case SDLK_p:
        if (is_menu_command(ev)) {
            if (ev.key.keysym.mod & KMOD_ALT) {
                if (!emu.palette().isDefault()) {
                    vs.commands.emplace(aldo::Command::paletteUnload);
                }
            } else {
                vs.commands.emplace(aldo::Command::paletteLoad);
            }
        }
        break;
    case SDLK_s:
        if (is_free_key(ev)) {
            vs.addProbeCommand(ALDO_INT_RST, !emu.probe(ALDO_INT_RST));
        }
        break;
    }
}

auto process_command(const aldo::command_state& cs, aldo::Emulator& emu,
                     aldo::viewstate& vs, const aldo::MediaRuntime& mr)
{
    auto& debugger = emu.debugger();
    auto breakpoints = debugger.breakpoints();
    switch (cs.cmd) {
    case aldo::Command::breakpointAdd:
        if (!breakpoints.append(std::get<aldo_haltexpr>(cs.value))) {
            // NOTE: breakpoint append can fail to allocate new memory
            breakpoint_add_failed();
        }
        break;
    case aldo::Command::breakpointDisable:
        breakpoints.disable(std::get<aldo::et::diff>(cs.value));
        break;
    case aldo::Command::breakpointEnable:
        breakpoints.enable(std::get<aldo::et::diff>(cs.value));
        break;
    case aldo::Command::breakpointRemove:
        breakpoints.remove(std::get<aldo::et::diff>(cs.value));
        break;
    case aldo::Command::breakpointsClear:
        breakpoints.clear();
        break;
    case aldo::Command::breakpointsExport:
        aldo::modal::exportBreakpoints(emu, mr);
        break;
    case aldo::Command::breakpointsOpen:
        aldo::modal::loadBreakpoints(emu, mr);
        break;
    case aldo::Command::mode:
        emu.runMode(std::get<aldo_execmode>(cs.value));
        break;
    case aldo::Command::openROM:
        if (aldo::modal::loadROM(emu, mr)) {
            vs.clock.resetEmu();
        }
        break;
    case aldo::Command::paletteLoad:
        aldo::modal::loadPalette(emu, mr);
        break;
    case aldo::Command::paletteUnload:
        emu.palette().unload();
        break;
    case aldo::Command::probe:
        {
            auto [signal, active] =
                std::get<aldo::command_state::probe>(cs.value);
            emu.probe(signal, active);
        }
        break;
    case aldo::Command::ready:
        emu.ready(std::get<bool>(cs.value));
        break;
    case aldo::Command::resetVectorClear:
        debugger.vectorClear();
        break;
    case aldo::Command::resetVectorOverride:
        debugger.vectorOverride(std::get<int>(cs.value));
        break;
    case aldo::Command::quit:
        vs.running = false;
        break;
    case aldo::Command::zeroRamOnPowerup:
        emu.zeroRam = std::get<bool>(cs.value);
        break;
    default:
        throw std::domain_error{invalid_command(cs.cmd)};
    }
}

}

void aldo::input::handle(aldo::Emulator& emu, aldo::viewstate& vs,
                         const aldo::MediaRuntime& mr)
{
    auto timer = vs.clock.timeInput();
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
        process_command(cs, emu, vs, mr);
        vs.commands.pop();
    }
}
