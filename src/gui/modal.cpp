//
//  modal.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/24/23.
//

#include "modal.hpp"

#include "debug.hpp"
#include "emu.hpp"
#include "error.hpp"
#include "guiplatform.h"
#include "handle.hpp"

#include <SDL2/SDL.h>

#include <filesystem>
#include <functional>
#include <initializer_list>
#include <iterator> // for std::data(); weird this isn't in <initializer_list>
#include <utility>

namespace
{

using modal_launch = std::function<std::filesystem::path(const gui_platform&)>;
using modal_operation = std::function<void(const std::filesystem::path&)>;

auto open_file(const gui_platform& p, const char* title,
               std::initializer_list<const char*> filter = {nullptr})
{
    const aldo::platform_buffer buf{
        p.open_file(title, std::data(filter)), p.free_buffer,
    };
    return buf ? std::filesystem::path{buf.get()} : std::filesystem::path{};
}

auto save_file(const gui_platform& p, const char* title,
               const std::filesystem::path& suggestedName)
{
    const aldo::platform_buffer buf{
        p.save_file(title, suggestedName.c_str()), p.free_buffer,
    };
    return buf ? std::filesystem::path{buf.get()} : std::filesystem::path{};
}

auto file_modal(modal_launch open, modal_operation op, aldo::Emulator& emu,
                const gui_platform& p)
{
    // NOTE: halt emulator to prevent time-jump from modal delay
    // TODO: does this make sense long-term?
    emu.halt();

    const auto filepath = open(p);
    if (filepath.empty()) return;

    SDL_Log("File selected: %s", filepath.c_str());
    try {
        op(filepath);
    } catch (const aldo::AldoError& err) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
        p.display_error(err.title(), err.message());
    }
}

}

void aldo::modal::loadROM(aldo::Emulator& emu, const gui_platform& p)
{
    file_modal([](const gui_platform& p) {
        return open_file(p, "Choose a ROM file");
    }, [&emu](const std::filesystem::path& p) { emu.loadCart(p); }, emu, p);
}

void aldo::modal::loadBreakpoints(aldo::Emulator& emu, const gui_platform& p)
{
    file_modal([](const gui_platform& p) {
        return open_file(p, "Choose a Breakpoints file",
                         {aldo::debug::BreakFileExtension, nullptr});
    }, [&d = emu.debugger()](const std::filesystem::path& p) {
        d.loadBreakpoints(p);
    }, emu, p);
}

void aldo::modal::exportBreakpoints(aldo::Emulator& emu, const gui_platform& p)
{
    file_modal([n = aldo::debug::breakfile_path_from(emu.cartName())]
               (const gui_platform& p) {
        return save_file(p, "Export Breakpoints", n);
    }, [&d = std::as_const(emu.debugger())](const std::filesystem::path& p) {
        d.exportBreakpoints(p);
    }, emu, p);
}
