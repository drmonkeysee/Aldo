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

namespace aldo
{

namespace colors
{

inline constexpr std::uint8_t ScreenFill = 0x1e;

inline constexpr ImU32
    Attention = IM_COL32(0xff, 0xa5, 0x0, SDL_ALPHA_OPAQUE),
    Destructive = IM_COL32(0x8b, 0x0, 0x0, SDL_ALPHA_OPAQUE),
    DestructiveActive = IM_COL32(0xb2, 0x22, 0x22, SDL_ALPHA_OPAQUE),
    DestructiveHover = IM_COL32(0xdc, 0x14, 0x3c, SDL_ALPHA_OPAQUE),
    LedOff = IM_COL32(0x43, 0x39, 0x36, SDL_ALPHA_OPAQUE),
    LedOn = IM_COL32(0xff, 0xfc, 0x53, SDL_ALPHA_OPAQUE),
    LineIn = IM_COL32(0x87, 0xce, 0xfa, SDL_ALPHA_OPAQUE),
    LineOut = IM_COL32(0x90, 0xee, 0x90, SDL_ALPHA_OPAQUE);

inline ImU32 white(bool enabled) noexcept
{
    if (enabled) return IM_COL32_WHITE;

    ImColor white{IM_COL32_WHITE};
    white.Value.w = ImGui::GetStyle().DisabledAlpha;
    return white;
}

}

namespace style
{

inline ImVec2 glyph_size() noexcept
{
    return ImGui::CalcTextSize("A");
}

}

}

#endif
