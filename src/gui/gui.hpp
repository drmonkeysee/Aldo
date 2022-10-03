//
//  gui.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/24/22.
//

#ifndef Aldo_gui_gui_hpp
#define Aldo_gui_gui_hpp

#include "options.hpp"

#ifdef __cplusplus
#define ALDO_NOEXCEPT noexcept
namespace aldo
{
extern "C"
{
#else
#define ALDO_NOEXCEPT
#endif

int aldo_rungui_with_args(int argc, char* argv[],
                          const struct aldo_guiopts* options) ALDO_NOEXCEPT;

#ifdef __cplusplus
}

int rungui_with_args(int argc, char* argv[], const guiopts& options);

}
#endif
#undef ALDO_NOEXCEPT

#endif
