//
//  palette.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 4/26/24.
//

#ifndef Aldo_gui_palette_hpp
#define Aldo_gui_palette_hpp

#include "imgui.h"

#include <filesystem>
#include <cstddef>

namespace aldo
{

class Palette {
public:
    using size = std::size_t;

    Palette() noexcept;
    explicit Palette(const std::filesystem::path& filepath);

    ImU32 getColor(size idx) const noexcept;
};

}

#endif
