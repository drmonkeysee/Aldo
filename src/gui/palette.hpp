//
//  palette.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 4/26/24.
//

#ifndef Aldo_gui_palette_hpp
#define Aldo_gui_palette_hpp

#include "imgui.h"
#include <SDL2/SDL.h>

#include <array>
#include <filesystem>
#include <memory>
#include <string_view>
#include <variant>

namespace aldo
{

namespace palette
{

inline constexpr const char
    * FileExtension = "pal",
    * Name = "Default";

// NOTE: 2C02 palette from https://www.nesdev.org/wiki/PPU_palettes
inline constexpr std::array Default {
    // 0x00
    IM_COL32(0x62, 0x62, 0x62, SDL_ALPHA_OPAQUE),   // #626262
    IM_COL32(0x1, 0x20, 0x90, SDL_ALPHA_OPAQUE),    // #012090
    IM_COL32(0x24, 0xb, 0xa0, SDL_ALPHA_OPAQUE),    // #240BA0
    IM_COL32(0x47, 0x0, 0x90, SDL_ALPHA_OPAQUE),    // #470090
    IM_COL32(0x60, 0x0, 0x62, SDL_ALPHA_OPAQUE),    // #600062
    IM_COL32(0x6a, 0x0, 0x24, SDL_ALPHA_OPAQUE),    // #6A0024
    IM_COL32(0x60, 0x11, 0x0, SDL_ALPHA_OPAQUE),    // #601100
    IM_COL32(0x47, 0x27, 0x0, SDL_ALPHA_OPAQUE),    // #472700
    IM_COL32(0x24, 0x3c, 0x0, SDL_ALPHA_OPAQUE),    // #243C00
    IM_COL32(0x1, 0x4a, 0x0, SDL_ALPHA_OPAQUE),     // #014A00
    IM_COL32(0x0, 0x4f, 0x0, SDL_ALPHA_OPAQUE),     // #004F00
    IM_COL32(0x0, 0x47, 0x24, SDL_ALPHA_OPAQUE),    // #004724
    IM_COL32(0x0, 0x36, 0x62, SDL_ALPHA_OPAQUE),    // #003662
    IM_COL32_BLACK,                                 // #000000
    IM_COL32_BLACK,                                 // #000000
    IM_COL32_BLACK,                                 // #000000

    // 0x10
    IM_COL32(0xab, 0xab, 0xab, SDL_ALPHA_OPAQUE),   // #ABABAB
    IM_COL32(0x1f, 0x56, 0xe1, SDL_ALPHA_OPAQUE),   // #1F56E1
    IM_COL32(0x4d, 0x39, 0xff, SDL_ALPHA_OPAQUE),   // #4D39FF
    IM_COL32(0x7e, 0x23, 0xef, SDL_ALPHA_OPAQUE),   // #7E23EF
    IM_COL32(0xa3, 0x1b, 0xb7, SDL_ALPHA_OPAQUE),   // #A31BB7
    IM_COL32(0xb4, 0x22, 0x64, SDL_ALPHA_OPAQUE),   // #B42264
    IM_COL32(0xac, 0x37, 0xe, SDL_ALPHA_OPAQUE),    // #AC370E
    IM_COL32(0x8c, 0x55, 0x0, SDL_ALPHA_OPAQUE),    // #8C5500
    IM_COL32(0x5e, 0x72, 0x0, SDL_ALPHA_OPAQUE),    // #5E7200
    IM_COL32(0x2d, 0x88, 0x0, SDL_ALPHA_OPAQUE),    // #2D8800
    IM_COL32(0x7, 0x90, 0x0, SDL_ALPHA_OPAQUE),     // #079000
    IM_COL32(0x0, 0x89, 0x47, SDL_ALPHA_OPAQUE),    // #008947
    IM_COL32(0x0, 0x73, 0x9d, SDL_ALPHA_OPAQUE),    // #00739D
    IM_COL32_BLACK,                                 // #000000
    IM_COL32_BLACK,                                 // #000000
    IM_COL32_BLACK,                                 // #000000

    // 0x20
    IM_COL32_WHITE,                                 // #FFFFFF
    IM_COL32(0x67, 0xac, 0xff, SDL_ALPHA_OPAQUE),   // #67ACFF
    IM_COL32(0x95, 0x8d, 0xff, SDL_ALPHA_OPAQUE),   // #958DFF
    IM_COL32(0xc8, 0x75, 0xff, SDL_ALPHA_OPAQUE),   // #C875FF
    IM_COL32(0xf2, 0x6a, 0xff, SDL_ALPHA_OPAQUE),   // #F26AFF
    IM_COL32(0xff, 0x6f, 0xc5, SDL_ALPHA_OPAQUE),   // #FF6FC5
    IM_COL32(0xff, 0x83, 0x6a, SDL_ALPHA_OPAQUE),   // #FF836A
    IM_COL32(0xe6, 0xa0, 0x1f, SDL_ALPHA_OPAQUE),   // #E6A01F
    IM_COL32(0xb8, 0xbf, 0x0, SDL_ALPHA_OPAQUE),    // #B8BF00
    IM_COL32(0x85, 0xd8, 0x1, SDL_ALPHA_OPAQUE),    // #85D801
    IM_COL32(0x5b, 0xe3, 0x35, SDL_ALPHA_OPAQUE),   // #5BE335
    IM_COL32(0x45, 0xde, 0x88, SDL_ALPHA_OPAQUE),   // #45DE88
    IM_COL32(0x49, 0xca, 0xe3, SDL_ALPHA_OPAQUE),   // #49CAE3
    IM_COL32(0x4e, 0x4e, 0x4e, SDL_ALPHA_OPAQUE),   // #4E4E4E
    IM_COL32_BLACK,                                 // #000000
    IM_COL32_BLACK,                                 // #000000

    // 0x30
    IM_COL32_WHITE,                                 // #FFFFFF
    IM_COL32(0xbf, 0xe0, 0xff, SDL_ALPHA_OPAQUE),   // #BFE0FF
    IM_COL32(0xd1, 0xd3, 0xff, SDL_ALPHA_OPAQUE),   // #D1D3FF
    IM_COL32(0xe6, 0xc9, 0xff, SDL_ALPHA_OPAQUE),   // #E6C9FF
    IM_COL32(0xf7, 0xc3, 0xff, SDL_ALPHA_OPAQUE),   // #F7C3FF
    IM_COL32(0xff, 0xc4, 0xee, SDL_ALPHA_OPAQUE),   // #FFC4EE
    IM_COL32(0xff, 0xcb, 0xc9, SDL_ALPHA_OPAQUE),   // #FFCBC9
    IM_COL32(0xf7, 0xd7, 0xa9, SDL_ALPHA_OPAQUE),   // #F7D7A9
    IM_COL32(0xe6, 0xe3, 0x97, SDL_ALPHA_OPAQUE),   // #E6E397
    IM_COL32(0xd1, 0xee, 0x97, SDL_ALPHA_OPAQUE),   // #D1EE97
    IM_COL32(0xbf, 0xf3, 0xa9, SDL_ALPHA_OPAQUE),   // #BFF3A9
    IM_COL32(0xb5, 0xf2, 0xc9, SDL_ALPHA_OPAQUE),   // #B5F2C9
    IM_COL32(0xb5, 0xeb, 0xee, SDL_ALPHA_OPAQUE),   // #B5EBEE
    IM_COL32(0xb8, 0xb8, 0xb8, SDL_ALPHA_OPAQUE),   // #B8B8B8
    IM_COL32_BLACK,                                 // #000000
    IM_COL32_BLACK,                                 // #000000
};
using sz = decltype(Default)::size_type;
using datav = decltype(Default)::value_type;
using datap = decltype(Default)::const_pointer;
inline constexpr sz Size = Default.size();

}

class Palette {
public:
    Palette() noexcept : colors{getDefault()} {}

    std::string_view name() const noexcept
    {
        if (filename.empty()) return palette::Name;
        return filename.native();
    }
    bool isDefault() const noexcept
    {
        return std::holds_alternative<palette::datap>(colors);
    }

    palette::datav getColor(palette::sz idx) const;

    void load(const std::filesystem::path& filepath);
    void unload() noexcept
    {
        colors = getDefault();
        filename.clear();
    }

private:
    static constexpr palette::datap getDefault() noexcept
    {
        return palette::Default.data();
    }

    std::filesystem::path filename;
    std::variant<
        palette::datap,
        std::unique_ptr<const palette::datav[]>
    > colors;
};

}

#endif
