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

ALDO_OWN
auto load_cart(const std::filesystem::path& filepath)
{
    using file_handle = aldo::handle<std::FILE, std::fclose>;

    aldo_cart* c;
    file_handle f{std::fopen(filepath.c_str(), "rb")};
    if (!f) throw aldo::AldoError{"Cannot open cart file", filepath, errno};

    int err = aldo_cart_create(&c, f.get());
    if (err < 0) {
        if (err == ALDO_CART_ERR_ERNO) throw aldo::AldoError{
            CartLoadFailure, "System error", errno,
        };
        throw aldo::AldoError{CartLoadFailure, err, aldo_cart_errstr};
    };

    return c;
}

}

//
// MARK: - Public Interface
//

aldo::Emulator::Emulator(aldo::debug_handle d, aldo::console_handle c,
                         const gui_platform& p)
: prefspath{get_prefspath(p)}, hdbg{std::move(d)}, hconsole{std::move(c)}
{
    aldo_nes_set_snapshot(consolep(), snapshotp());
}

std::string_view aldo::Emulator::displayCartName() const noexcept
{
    if (cartName().empty()) return aldo_cart_errstr(ALDO_CART_ERR_NOCART);

    return cartName().native();
}

std::optional<aldo_cartinfo> aldo::Emulator::cartInfo() const
{
    if (!hcart) return {};

    aldo_cartinfo info;
    aldo_cart_getinfo(cartp(), &info);
    return info;
}

void aldo::Emulator::loadCart(const std::filesystem::path& filepath)
{
    auto c = load_cart(filepath);
    saveCartState();
    aldo_nes_powerdown(consolep());
    hcart.reset(c);
    aldo_nes_powerup(consolep(), cartp(), zeroRam);
    cartpath = filepath;
    cartname = cartpath.stem();
    loadCartState();
}

void aldo::Emulator::update(aldo::viewstate& vs) noexcept
{
    auto timer = vs.clock.timeUpdate();
    aldo_nes_clock(consolep(), vs.clock.clockp());
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
    if (!hcart) return;

    debugger().saveCartState(prefspath / cartName());
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
    aldo_nes_set_snapshot(consolep(), nullptr);
}
