//
//  emu.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/23.
//

#include "emu.hpp"

#include "attr.hpp"
#include "guiplatform.h"
#include "viewstate.hpp"

#include <exception>
#include <utility>
#include <cstdio>

namespace
{

constexpr const char* CartLoadFailure = "Cart load failure";

auto get_prefspath(const gui_platform& p)
{
    using sdl_buffer = aldo::handle<char, SDL_free>;

    aldo::platform_buffer
        org{p.orgname(), p.free_buffer},
        name{p.appname(), p.free_buffer};
    sdl_buffer path{SDL_GetPrefPath(org.get(), name.get())};
    if (!path) throw aldo::SdlError{"Failed to get preferences path"};
    return std::filesystem::path{path.get()};
}

auto load_cart(const std::filesystem::path& filepath)
{
    using file_handle = aldo::handle<std::FILE, std::fclose>;

    aldo_cart* c;
    file_handle f{std::fopen(filepath.c_str(), "rb")};
    if (!f) throw aldo::AldoError{"Cannot open cart file", filepath, errno};

    auto err = aldo_cart_create(&c, f.get());
    if (err < 0) {
        if (err == ALDO_CART_ERR_ERNO) throw aldo::AldoError{
            CartLoadFailure, "System error", errno,
        };
        throw aldo::AldoError{CartLoadFailure, err, aldo_cart_errstr};
    };

    return aldo::cart_handle{c};
}

}

//
// MARK: - Public Interface
//

aldo::Emulator::Emulator(aldo::debug_handle d, aldo::cart_handle c,
                         aldo::console_handle cn, const gui_platform& p)
: prefspath{get_prefspath(p)}, hdbg{std::move(d)}, hcart{std::move(c)},
    hconsole{std::move(cn)} {}

void aldo::Emulator::loadCart(const std::filesystem::path& filepath)
{
    auto c = load_cart(filepath);
    saveCartState();
    reloadConsole(c.get());
    hcart.swap(c);
    cartpath = filepath;
    cartname = cartpath.stem();
    loadCartState();
}

void aldo::Emulator::update(aldo::viewstate& vs) noexcept
{
    auto timer = vs.clock.timeUpdate();
    aldo_console_clock(consolep(), vs.clock.clockp());
}

//
// MARK: - Private Interface
//

void aldo::Emulator::loadCartState()
{
    debugger().loadCartState(prefspath / cartName());
}


void aldo::Emulator::saveCartState() const
{
    debugger().saveCartState(prefspath / cartName());
}

void aldo::Emulator::reloadConsole(aldo_cart* c)
{
    auto cn = hconsole.release();
    auto success = aldo_console_poweron(&cn, c, hdbg.dbgp(), nullptr, false);
    hconsole.reset(cn);
    if (!success) throw aldo::AldoError{
        "Console load failure", "System error", errno,
    };
}

void aldo::Emulator::cleanup() const noexcept
{
    try {
        saveCartState();
    } catch (const std::exception& ex) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Emu dtor exception: %s",
                     ex.what());
    } catch (...) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unknown Emu dtor error!");
    }
}
