//
//  uiimgui.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/6/22.
//

#ifndef Aldo_gui_uiimgui_hpp
#define Aldo_gui_uiimgui_hpp

#include <SDL2/SDL.h>

#ifdef __cplusplus
namespace aldo
{
extern "C"
{
#define ALDO_NOEXCEPT noexcept
#else
#define ALDO_NOEXCEPT
#endif

void imgui_handle_input(SDL_Event *ev) ALDO_NOEXCEPT;

#ifdef __cplusplus
}
}
#endif
#undef ALDO_NOEXCEPT

#endif
