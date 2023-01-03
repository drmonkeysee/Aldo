//
//  style.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/29/22.
//

#ifndef Aldo_gui_style_hpp
#define Aldo_gui_style_hpp

#include "imgui.h"
#include <SDL2/SDL.h>

#include <cstdint>

namespace aldo::colors
{

inline constexpr std::uint8_t ScreenFill = 0x1e;

inline constexpr ImU32
    Attention = IM_COL32(0xff, 0xa5, 0x0, SDL_ALPHA_OPAQUE),
    Destructive = IM_COL32(0x8b, 0x0, 0x0, SDL_ALPHA_OPAQUE),
    DestructiveActive = IM_COL32(0xb2, 0x22, 0x22, SDL_ALPHA_OPAQUE),
    DestructiveHover = IM_COL32(0xdc, 0x14, 0x3c, SDL_ALPHA_OPAQUE),
    LedOff = IM_COL32(0x43, 0x39, 0x36, SDL_ALPHA_OPAQUE),
    LedOn = IM_COL32(0xff, 0xfc, 0x53, SDL_ALPHA_OPAQUE);

}

#endif
