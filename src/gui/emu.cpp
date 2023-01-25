//
//  emu.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/23/23.
//

#include "emu.hpp"

#include "error.hpp"

#include <cerrno>
#include <cstdio>

namespace
{

using file_handle = aldo::handle<std::FILE, std::fclose>;

}

void aldo::Emulator::loadCart(const std::filesystem::path& filepath)
{
    cart* c;
    errno = 0;
    const file_handle f{std::fopen(filepath.c_str(), "rb")};
    if (!f) throw aldo::AldoError{"Cannot open cart file", filepath, errno};

    const int err = cart_create(&c, f.get());
    if (err < 0) throw aldo::AldoError{"Cart load failure", err, cart_errstr};

    //saveCartState(p);
    nes_powerdown(consolep());
    hcart.reset(c);
    nes_powerup(consolep(), cartp(), false);
    cartpath = filepath;
    cartname = cartpath.stem();
    //loadCartState(p);
}
