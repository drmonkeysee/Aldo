//
//  emutypes.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 1/7/23.
//

#ifndef Aldo_gui_emutypes_hpp
#define Aldo_gui_emutypes_hpp

#include <cstddef>
#include <cstdint>

namespace aldo
{

struct emulator_types {
    using byte = std::uint8_t;
    using word = std::uint16_t;
    using size = std::size_t;
    using diff = std::ptrdiff_t;

    emulator_types() = delete;
};

using et = emulator_types;

}

#endif
