//
//  palette.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 4/26/24.
//

#include "palette.hpp"

#include "emutypes.hpp"
#include "error.hpp"

#include <fstream>
#include <ios>
#include <iterator>
#include <ranges>
#include <tuple>

namespace
{

constexpr aldo::palette::sz Channels = 3;
constexpr aldo::palette::sz PalFileLength = aldo::palette::Size * Channels;

using pal_buf = std::array<std::ifstream::char_type, PalFileLength>;

auto parse_colors(const pal_buf& buf)
{
    auto newColors =
        std::make_unique<aldo::palette::datav[]>(aldo::palette::Size);
    aldo::palette::sz idx = 0;

    // TODO: replace this with views::chunk in c++23
    const auto end = buf.cend();
    for (auto it = buf.cbegin();
         it != end;
         std::advance(it, Channels)) {
        const auto t = std::views::take(std::ranges::subrange{it, end},
                                        Channels);
        // NOTE: t yields possibly signed chars; cast to byte, because int
        // auto-promotion in IM_COL32 will sign-extend and mess up the shifts.
        const auto [r, g, b] = std::tuple{
            static_cast<aldo::et::byte>(t[0]),
            static_cast<aldo::et::byte>(t[1]),
            static_cast<aldo::et::byte>(t[2]),
        };
        newColors[idx++] = IM_COL32(r, g, b, SDL_ALPHA_OPAQUE);
    }

    return newColors;
}

}

//
// Public Interface
//

aldo::palette::datav aldo::Palette::getColor(aldo::palette::sz idx) const
{
    return std::visit([idx](auto&& c) { return c[idx]; }, colors);
}

void aldo::Palette::load(const std::filesystem::path& filepath)
{
    std::ifstream f{filepath, std::ios::binary};
    if (!f) throw aldo::AldoError{"Cannot open palette file", filepath};

    pal_buf buf{};
    f.read(buf.data(), buf.size());
    if (f.bad() || (f.fail() && !f.eof())) throw aldo::AldoError{
        "Palette File Error", "Error reading palette file"
    };
    if (f.fail() && f.eof()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Palette file too short;"
                    " expected %zu bytes, read %zu bytes",
                    buf.size(), f.gcount());
    }

    colors = parse_colors(buf);
}
