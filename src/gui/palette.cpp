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

aldo::palette::datav aldo::Palette::getColor(aldo::palette::sz idx) const
{
    return std::visit([idx](auto&& c) { return c[idx]; }, colors);
}

void aldo::Palette::load(const std::filesystem::path&)
{
    // TODO: not implemented
    throw std::runtime_error{"not implemented"};
}
