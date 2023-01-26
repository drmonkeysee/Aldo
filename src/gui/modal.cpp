//
//  modal.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/24/23.
//

#include "modal.hpp"

#include "emu.hpp"
#include "error.hpp"
#include "guiplatform.h"
#include "handle.hpp"

#include <SDL2/SDL.h>

#include <filesystem>
#include <functional>
#include <initializer_list>

#include <span> // for std::data()?

namespace
{

using modal_launch = std::function<std::filesystem::path(const gui_platform&)>;
using modal_operation =
    std::function<void(const std::filesystem::path&, aldo::Emulator&)>;

auto open_file(const gui_platform& p, const char* title,
               std::initializer_list<const char*> filter = {nullptr})
{
    const aldo::platform_buffer buf{
        p.open_file(title, std::data(filter)), p.free_buffer,
    };
    return std::filesystem::path{buf.get()};
}

auto save_file(const gui_platform& p, const char* title,
               const std::filesystem::path& suggestedName)
{
    const aldo::platform_buffer buf{
        p.save_file(title, suggestedName.c_str()), p.free_buffer,
    };
    return std::filesystem::path{buf.get()};
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
        op(filepath, emu);
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
    }, [](const std::filesystem::path& p, aldo::Emulator& e) {
        e.loadCart(p);
    }, emu, p);
}

void aldo::modal::loadBreakpoints(const aldo::Emulator& emu,
                                  const gui_platform& p)
{
    /*
     const auto open =
         [](const gui_platform& p) -> aldo::platform_buffer {
             return open_file(p, "Choose a Breakpoints file",
                              {EXT_BRK, nullptr});
         };
     openModal(p, {open, &aldo::EmuController::loadBreakpointsFrom});
     */
}

void aldo::modal::exportBreakpoints(const aldo::Emulator& emu,
                                    const gui_platform& p)
{
    /*
     const auto open =
         [this](const gui_platform& p) -> aldo::platform_buffer {
             return save_file(p, "Export Breakpoints",
                              brkfile_name(this->cartname));
         };
     openModal(p, {open, &aldo::EmuController::exportBreakpointsTo});
     */
}
