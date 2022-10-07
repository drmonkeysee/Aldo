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
#define ALDO_NOARG
#else
#include <stdbool.h>
#define ALDO_NOEXCEPT
#define ALDO_NOARG void
#endif

bool imgui_init(SDL_Window *win, SDL_Renderer *ren) ALDO_NOEXCEPT;
void imgui_handle_input(SDL_Event *ev) ALDO_NOEXCEPT;
void imgui_render(bool *show_demo) ALDO_NOEXCEPT;
void imgui_commit_render(ALDO_NOARG) ALDO_NOEXCEPT;
void imgui_cleanup(ALDO_NOARG) ALDO_NOEXCEPT;

#ifdef __cplusplus
}
}
#endif
#undef ALDO_NOARG
#undef ALDO_NOEXCEPT

#endif
