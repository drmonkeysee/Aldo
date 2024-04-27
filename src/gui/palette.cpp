//
//  palette.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 4/26/24.
//

#include "palette.hpp"

#include <stdexcept>

//
// Public Interface
//

void aldo::Palette::load(const std::filesystem::path&)
{
    // TODO: not implemented
    throw new std::runtime_error{"not implemented"};
}

aldo::palette::datav aldo::Palette::getColor(aldo::palette::size idx) const
{
    return std::visit([idx](auto&& c) { return c[idx]; }, colors);
}
