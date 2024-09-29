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

#include <algorithm>
#include <tuple>
#include <cstdint>

namespace aldo
{

namespace colors
{

inline constexpr Uint8 ScreenFill = 0x1e;

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

inline constexpr ImU32 color_channel(ImU32 color, ImU32 shift) noexcept
{
    return (color >> shift) & 0xff;
}

inline constexpr ImU32 rch(ImU32 color) noexcept
{
    return color_channel(color, IM_COL32_R_SHIFT);
}

inline constexpr ImU32 gch(ImU32 color) noexcept
{
    return color_channel(color, IM_COL32_G_SHIFT);
}

inline constexpr ImU32 bch(ImU32 color) noexcept
{
    return color_channel(color, IM_COL32_B_SHIFT);
}

inline constexpr std::tuple<ImU32, ImU32, ImU32> rgb(ImU32 color)
{
    return {rch(color), gch(color), bch(color)};
}

inline constexpr std::tuple<ImU32, ImU32, ImU32> rgb_floor(ImU32 color,
                                                           ImU32 floor)
{
    return {
        std::max(floor, rch(color)),
        std::max(floor, gch(color)),
        std::max(floor, bch(color)),
    };
}

// NOTE: relative luminance function
// https://en.wikipedia.org/wiki/Relative_luminance
inline constexpr ImU32 luminance(ImU32 color)
{
    auto [r, g, b] = rgb(color);
    return static_cast<ImU32>((0.2126f * static_cast<float>(r))
                              + (0.7152f * static_cast<float>(g))
                              + (0.0722f * static_cast<float>(b)));
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
