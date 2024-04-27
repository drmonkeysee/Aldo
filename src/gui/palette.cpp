//
//  palette.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 4/26/24.
//

#include "palette.hpp"

#include "imgui.h"
#include <SDL2/SDL.h>

#include <array>
#include <stdexcept>

namespace
{

// NOTE: 2C02 palette from https://www.nesdev.org/wiki/PPU_palettes
constexpr std::array DefaultPalette {
    // 0x00
    IM_COL32(0x62, 0x62, 0x62, SDL_ALPHA_OPAQUE),   // #626262
    IM_COL32(0x0, 0x1f, 0xb2, SDL_ALPHA_OPAQUE),    // #001FB2
    IM_COL32(0x24, 0x4, 0xc8, SDL_ALPHA_OPAQUE),    // #2404C8
    IM_COL32(0x52, 0x0, 0xb2, SDL_ALPHA_OPAQUE),    // #5200B2
    IM_COL32(0x73, 0x0, 0x76, SDL_ALPHA_OPAQUE),    // #730076
    IM_COL32(0x80, 0x0, 0x24, SDL_ALPHA_OPAQUE),    // #800024
    IM_COL32(0x73, 0xb, 0x0, SDL_ALPHA_OPAQUE),     // #730B00
    IM_COL32(0x52, 0x28, 0x0, SDL_ALPHA_OPAQUE),    // #522800
    IM_COL32(0x24, 0x44, 0x0, SDL_ALPHA_OPAQUE),    // #244400
    IM_COL32(0x0, 0x57, 0x0, SDL_ALPHA_OPAQUE),     // #005700
    IM_COL32(0x0, 0x5c, 0x0, SDL_ALPHA_OPAQUE),     // #005C00
    IM_COL32(0x0, 0x53, 0x24, SDL_ALPHA_OPAQUE),    // #005324
    IM_COL32(0x0, 0x3c, 0x76, SDL_ALPHA_OPAQUE),    // #003C76
    IM_COL32_BLACK,                                 // #000000
    IM_COL32_BLACK,                                 // #000000
    IM_COL32_BLACK,                                 // #000000

    // 0x10
    IM_COL32(0xab, 0xab, 0xab, SDL_ALPHA_OPAQUE),   // #ABABAB
    IM_COL32(0xd, 0x57, 0xff, SDL_ALPHA_OPAQUE),    // #0D57FF
    IM_COL32(0x4b, 0x30, 0xff, SDL_ALPHA_OPAQUE),   // #4B30FF
    IM_COL32(0x8a, 0x13, 0xff, SDL_ALPHA_OPAQUE),   // #8A13FF
    IM_COL32(0xbc, 0x8, 0xd6, SDL_ALPHA_OPAQUE),    // #BC08D6
    IM_COL32(0xd2, 0x12, 0x69, SDL_ALPHA_OPAQUE),   // #D21269
    IM_COL32(0xc7, 0x2e, 0x0, SDL_ALPHA_OPAQUE),    // #C72E00
    IM_COL32(0x9d, 0x54, 0x0, SDL_ALPHA_OPAQUE),    // #9D5400
    IM_COL32(0x60, 0x7b, 0x0, SDL_ALPHA_OPAQUE),    // #607B00
    IM_COL32(0x20, 0x98, 0x0, SDL_ALPHA_OPAQUE),    // #209800
    IM_COL32(0x0, 0xa3, 0x0, SDL_ALPHA_OPAQUE),     // #00A300
    IM_COL32(0x0, 0x99, 0x42, SDL_ALPHA_OPAQUE),    // #009942
    IM_COL32(0x0, 0x7d, 0xb4, SDL_ALPHA_OPAQUE),    // #007DB4
    IM_COL32_BLACK,                                 // #000000
    IM_COL32_BLACK,                                 // #000000
    IM_COL32_BLACK,                                 // #000000

    // 0x20
    IM_COL32_WHITE,                                 // #FFFFFF
    IM_COL32(0x53, 0xae, 0xff, SDL_ALPHA_OPAQUE),   // #53AEFF
    IM_COL32(0x90, 0x85, 0xff, SDL_ALPHA_OPAQUE),   // #9085FF
    IM_COL32(0xd3, 0x65, 0xff, SDL_ALPHA_OPAQUE),   // #D365FF
    IM_COL32(0xff, 0x57, 0xff, SDL_ALPHA_OPAQUE),   // #FF57FF
    IM_COL32(0xff, 0x5d, 0xcf, SDL_ALPHA_OPAQUE),   // #FF5DCF
    IM_COL32(0xff, 0x77, 0x57, SDL_ALPHA_OPAQUE),   // #FF7757
    IM_COL32(0xfa, 0x9e, 0x0, SDL_ALPHA_OPAQUE),    // #FA9E00
    IM_COL32(0xbd, 0xc7, 0x0, SDL_ALPHA_OPAQUE),    // #BDC700
    IM_COL32(0x7a, 0xe7, 0x0, SDL_ALPHA_OPAQUE),    // #7AE700
    IM_COL32(0x43, 0xf6, 0x11, SDL_ALPHA_OPAQUE),   // #43F611
    IM_COL32(0x26, 0xef, 0x7e, SDL_ALPHA_OPAQUE),   // #26EF7E
    IM_COL32(0x2c, 0xd5, 0xf6, SDL_ALPHA_OPAQUE),   // #2CD5F6
    IM_COL32(0x4e, 0x4e, 0x4e, SDL_ALPHA_OPAQUE),   // #4E4E4E
    IM_COL32_BLACK,                                 // #000000
    IM_COL32_BLACK,                                 // #000000

    // 0x30
    IM_COL32_WHITE,                                 // #FFFFFF
    IM_COL32(0xb6, 0xe1, 0xff, SDL_ALPHA_OPAQUE),   // #B6E1FF
    IM_COL32(0xce, 0xd1, 0xff, SDL_ALPHA_OPAQUE),   // #CED1FF
    IM_COL32(0xe9, 0xc3, 0xff, SDL_ALPHA_OPAQUE),   // #E9C3FF
    IM_COL32(0xff, 0xbc, 0xff, SDL_ALPHA_OPAQUE),   // #FFBCFF
    IM_COL32(0xff, 0xbd, 0xf4, SDL_ALPHA_OPAQUE),   // #FFBDF4
    IM_COL32(0xff, 0xc6, 0xc3, SDL_ALPHA_OPAQUE),   // #FFC6C3
    IM_COL32(0xff, 0xd5, 0x9a, SDL_ALPHA_OPAQUE),   // #FFD59A
    IM_COL32(0xe9, 0xe6, 0x81, SDL_ALPHA_OPAQUE),   // #E9E681
    IM_COL32(0xce, 0xf4, 0x81, SDL_ALPHA_OPAQUE),   // #CEF481
    IM_COL32(0xb6, 0xfb, 0x9a, SDL_ALPHA_OPAQUE),   // #B6FB9A
    IM_COL32(0xa9, 0xfa, 0xc3, SDL_ALPHA_OPAQUE),   // #A9FAC3
    IM_COL32(0xa9, 0xf0, 0xf4, SDL_ALPHA_OPAQUE),   // #A9F0F4
    IM_COL32(0xb8, 0xb8, 0xb8, SDL_ALPHA_OPAQUE),   // #B8B8B8
    IM_COL32_BLACK,                                 // #000000
    IM_COL32_BLACK,                                 // #000000
};
using palette_sz = decltype(DefaultPalette)::size_type;
constexpr palette_sz PaletteSize = DefaultPalette.size();

}

//
// Public Interface
//

aldo::Palette::Palette() noexcept {}

aldo::Palette::Palette(const std::filesystem::path&)
{
    // TODO: not implemented
    throw new std::runtime_error{"not implemented"};
}
