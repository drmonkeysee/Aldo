//
//  palette.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 4/26/24.
//

#include "palette.hpp"

#include "error.hpp"
#include "style.hpp"

#include <fstream>
#include <ios>
#include <iterator>
#include <ranges>
#include <tuple>

namespace
{

constexpr aldo::palette::sz
    Channels = 3, PalFileLength = aldo::palette::Size * Channels;
constexpr float Attenuated = 0.816328f, Full = 1.0f;
// NOTE: minimum channel value used for applying emphasis in order to get a
// visible tint on colors that would otherwise round down to nearly zero.
constexpr ImU32 EmphasisFloor = 0x10;

using pal_buf = std::array<std::ifstream::char_type, PalFileLength>;

auto parse_colors(const pal_buf& buf)
{
    auto newColors =
        std::make_unique<aldo::palette::datav[]>(aldo::palette::Size);
    aldo::palette::sz idx = 0;

    // TODO: replace this with views::chunk in c++23
    auto end = buf.cend();
    for (auto it = buf.cbegin();
         it != end;
         std::advance(it, Channels)) {
        auto t = std::views::take(std::ranges::subrange{it, end}, Channels);
        // NOTE: t yields possibly signed chars; cast to unsigned, because int
        // auto-promotion in IM_COL32 will sign-extend and mess up the shifts.
        auto [r, g, b] = std::tuple{
            static_cast<unsigned char>(t[0]),
            static_cast<unsigned char>(t[1]),
            static_cast<unsigned char>(t[2]),
        };
        newColors[idx++] = IM_COL32(r, g, b, SDL_ALPHA_OPAQUE);
    }

    return newColors;
}

}

//
// MARK: - Public Interface
//

aldo::palette::datav aldo::Palette::getColor(aldo::palette::sz idx,
                                             aldo::palette::emphasis em) const
{
    if (em.gs) {
        idx &= 0x30;
    }
    auto color = std::visit([idx](auto&& c) { return c[idx]; }, colors);
    if (em.any() && ((idx & 0xe) < 0xe)) {
        auto [r, g, b] = aldo::colors::rgb_floor(color, EmphasisFloor);
        auto [re, ge, be] = std::tuple{
            em.g || em.b ? Attenuated : Full,
            em.r || em.b ? Attenuated : Full,
            em.r || em.g ? Attenuated : Full,
        };
        color = IM_COL32(static_cast<float>(r) * re,
                         static_cast<float>(g) * ge,
                         static_cast<float>(b) * be,
                         SDL_ALPHA_OPAQUE);
    }
    return color;
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
    filename = filepath.filename();
}
