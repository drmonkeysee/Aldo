//
//  cart.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/3/22.
//

#ifndef Aldo_gui_cart_hpp
#define Aldo_gui_cart_hpp

#include "cart.h"
#include "handle.hpp"

#include <string>
#include <string_view>

namespace aldo
{

using cart_handle = handle<cart, cart_free>;

class Cart final {
public:
    void loadFrom(std::string_view filepath) noexcept;

    std::string_view name() const noexcept;
    cart* get() const noexcept;

private:
    std::string filepath;
    cart_handle handle;
};

}

#endif
