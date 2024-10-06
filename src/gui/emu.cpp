//
//  emu.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/23.
//

#include "emu.hpp"

#include "attr.hpp"
#include "error.hpp"
#include "guiplatform.h"
#include "viewstate.hpp"

#include <SDL2/SDL.h>

#include <exception>
#include <utility>
#include <cerrno>
#include <cstdio>

namespace
{

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

    cart* c;
    file_handle f{std::fopen(filepath.c_str(), "rb")};
    if (!f) throw aldo::AldoError{"Cannot open cart file", filepath, errno};

    int err = cart_create(&c, f.get());
    if (err < 0) throw aldo::AldoError{"Cart load failure", err, cart_errstr};

    return c;
}

auto update_bouncer(aldo::viewstate& vs, const aldo_snapshot& snp) noexcept
{
    if (!snp.cpu.lines.ready) return;

    if (vs.bouncer.pos.x - vs.bouncer.halfdim < 0
        || vs.bouncer.pos.x + vs.bouncer.halfdim > vs.bouncer.bounds.x) {
        vs.bouncer.velocity.x *= -1;
    }
    if (vs.bouncer.pos.y - vs.bouncer.halfdim < 0
        || vs.bouncer.pos.y + vs.bouncer.halfdim > vs.bouncer.bounds.y) {
        vs.bouncer.velocity.y *= -1;
    }
    vs.bouncer.pos.x += vs.bouncer.velocity.x;
    vs.bouncer.pos.y += vs.bouncer.velocity.y;
}

}

//
// MARK: - Public Interface
//

aldo::Emulator::Emulator(aldo::debug_handle d, aldo::console_handle n,
                         const gui_platform& p)
: prefspath{get_prefspath(p)}, hdbg{std::move(d)}, hconsole{std::move(n)},
    hsnp{consolep()} {}

std::string_view aldo::Emulator::displayCartName() const noexcept
{
    if (cartName().empty()) return cart_errstr(ALDO_CART_ERR_NOCART);

    return cartName().native();
}

std::optional<cartinfo> aldo::Emulator::cartInfo() const
{
    if (!hcart) return {};

    cartinfo info;
    cart_getinfo(cartp(), &info);
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
    aldo_nes_clock(consolep(), vs.clock.clockp());
    aldo_nes_snapshot(consolep(), snapshotp());
    update_bouncer(vs, snapshot());
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
}
