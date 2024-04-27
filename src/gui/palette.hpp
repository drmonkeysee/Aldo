//
//  palette.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 4/26/24.
//

#ifndef Aldo_gui_palette_hpp
#define Aldo_gui_palette_hpp

#include <filesystem>

namespace aldo
{

class Palette {
public:
    Palette() noexcept;
    explicit Palette(const std::filesystem::path& filepath);
};

}

#endif
