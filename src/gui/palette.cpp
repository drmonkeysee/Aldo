//
//  palette.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 4/26/24.
//

#include "palette.hpp"

#include "error.hpp"

#include <fstream>
#include <ios>

namespace
{

constexpr aldo::palette::sz PalFileLength = aldo::palette::Size * 3;

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

    std::array<std::ifstream::char_type, PalFileLength> buf{};
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
}
