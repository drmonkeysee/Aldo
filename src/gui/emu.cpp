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

#include <utility>
#include <cerrno>
#include <cstdio>

namespace
{

using file_handle = aldo::handle<std::FILE, std::fclose>;
using sdl_buffer = aldo::handle<char, SDL_free>;

auto get_prefspath(const gui_platform& p)
{
    const aldo::platform_buffer
        org{p.orgname(), p.free_buffer},
        name{p.appname(), p.free_buffer};
    const sdl_buffer path{SDL_GetPrefPath(org.get(), name.get())};
    if (!path) throw aldo::SdlError{"Failed to get preferences path"};
    return std::filesystem::path{path.get()};
}

ALDO_OWN
auto load_cart(const std::filesystem::path& filepath)
{
    cart* c;
    errno = 0;
    const file_handle f{std::fopen(filepath.c_str(), "rb")};
    if (!f) throw aldo::AldoError{"Cannot open cart file", filepath, errno};

    const int err = cart_create(&c, f.get());
    if (err < 0) throw aldo::AldoError{"Cart load failure", err, cart_errstr};

    return c;
}

auto update_bouncer(aldo::viewstate& vs,
                    const console_state& snapshot) noexcept
{
    if (!snapshot.lines.ready) return;

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
// Public Interface
//

aldo::Emulator::Emulator(Debugger d, console_handle n, const gui_platform& p)
: prefspath{get_prefspath(p)}, hdebug{std::move(d)}, hconsole{std::move(n)},
    hsnapshot{consolep()} {}

void aldo::Emulator::loadCart(const std::filesystem::path& filepath)
{
    const auto c = load_cart(filepath);
    saveCartState();
    nes_powerdown(consolep());
    hcart.reset(c);
    nes_powerup(consolep(), cartp(), false);
    cartpath = filepath;
    cartname = cartpath.stem();
    loadCartState();
}

void aldo::Emulator::update(aldo::viewstate& vs) noexcept
{
    nes_cycle(consolep(), &vs.clock.cyclock);
    nes_snapshot(consolep(), snapshotp());
    update_bouncer(vs, snapshot());
}

void aldo::Emulator::shutdown() const
{
    saveCartState();
}

//
// Private Interface
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
