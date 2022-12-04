//
//  cart.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/3/22.
//

#include "cart.hpp"

void aldo::Cart::loadFrom(std::string_view filepath) noexcept
{
    this->filepath = filepath;
}

std::string_view aldo::Cart::name() const
{
    if (filepath.empty()) return "NOCART";

    std::string_view v = filepath;
    auto slash = v.rfind('/');
    const auto dot = v.rfind('.');
    if (dot < slash) return v;

    if (slash == std::string_view::npos) {
        slash = 0;
    } else {
        ++slash;    // NOTE: one-past the last slash
    }
    // NOTE: if not found dot will be npos which will work for end-of-string,
    // even with slash subtracted.
    return v.substr(slash, dot - slash);
}
