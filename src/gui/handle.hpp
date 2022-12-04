//
//  handle.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 12/3/22.
//

#ifndef Aldo_gui_handle_hpp
#define Aldo_gui_handle_hpp

#include <memory>
#include <type_traits>

namespace aldo
{

template<auto f>
using func_deleter = std::integral_constant<std::decay_t<decltype(f)>, f>;

template<typename T, auto f>
using handle = std::unique_ptr<T, func_deleter<f>>;

}

#endif
