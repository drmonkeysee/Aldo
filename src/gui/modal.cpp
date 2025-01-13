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
#include "mediaruntime.hpp"

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
    aldo::platform_buffer buf{
        p.open_file(title, std::data(filter)), p.free_buffer,
    };
    return buf ? std::filesystem::path{buf.get()} : std::filesystem::path{};
}

auto save_file(const gui_platform& p, const char* title,
               const std::filesystem::path& suggestedName)
{
    aldo::platform_buffer buf{
        p.save_file(title, suggestedName.c_str()), p.free_buffer,
    };
    return buf ? std::filesystem::path{buf.get()} : std::filesystem::path{};
}

auto file_modal(modal_launch open, modal_operation op, aldo::Emulator& emu,
                const aldo::MediaRuntime& mr)
{
    // NOTE: halt emulator to prevent time-jump from modal delay
    // TODO: does this make sense long-term?
    emu.ready(false);

    auto filepath = open(mr.platform());
    // NOTE: at least on macOS main window doesn't auto-focus back from modal
    SDL_RaiseWindow(mr.window());
    if (filepath.empty()) return false;

    SDL_Log("File selected: %s", filepath.c_str());
    try {
        op(filepath);
        return true;
    } catch (const aldo::AldoError& err) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, err.title(),
                                 err.message(), nullptr);
    }
    return false;
}

}

bool aldo::modal::loadROM(aldo::Emulator& emu, const aldo::MediaRuntime& mr)
{
    auto open = [](const gui_platform& p) static {
        return open_file(p, "Choose a ROM file");
    };
    auto op = [&emu](const std::filesystem::path& fp) { emu.loadCart(fp); };
    return file_modal(open, op, emu, mr);
}

bool aldo::modal::loadBreakpoints(aldo::Emulator& emu,
                                  const aldo::MediaRuntime& mr)
{
    auto open = [](const gui_platform& p) static {
        return open_file(p, "Choose a Breakpoints file",
                         {aldo::debug::BreakFileExtension, nullptr});
    };
    auto op = [&d = emu.debugger()](const std::filesystem::path& fp) {
        d.loadBreakpoints(fp);
    };
    return file_modal(open, op, emu, mr);
}

bool aldo::modal::exportBreakpoints(aldo::Emulator& emu,
                                    const aldo::MediaRuntime& mr)
{
    auto open =
        [n = aldo::debug::breakfile_path_from(emu.cartName())]
        (const gui_platform& p) {
            return save_file(p, "Export Breakpoints", n);
        };
    auto op =
        [&d = std::as_const(emu.debugger())](const std::filesystem::path& fp) {
            d.exportBreakpoints(fp);
        };
    return file_modal(open, op, emu, mr);
}

bool aldo::modal::loadPalette(aldo::Emulator& emu,
                              const aldo::MediaRuntime& mr)
{
    auto open = [](const gui_platform& p) static {
        return open_file(p, "Choose a Palette",
                         {aldo::palette::FileExtension, nullptr});
    };
    auto op = [&p = emu.palette()](const std::filesystem::path& fp) {
        p.load(fp);
    };
    return file_modal(open, op, emu, mr);
}
