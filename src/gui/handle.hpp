//
//  handle.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/3/22.
//

#ifndef Aldo_gui_handle_hpp
#define Aldo_gui_handle_hpp

#include "guiplatform.h"

#include <concepts>
#include <memory>
#include <type_traits>

namespace aldo
{

template<auto f>
using func_wrapper = std::integral_constant<std::decay_t<decltype(f)>, f>;
template<typename T, std::invocable<T*> auto f>
using handle = std::unique_ptr<T, func_wrapper<f>>;

using platform_deleter = std::decay_t<decltype(gui_platform::free_buffer)>;
using platform_buffer = std::unique_ptr<char, platform_deleter>;

}

#endif
