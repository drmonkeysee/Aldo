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
#define ALDO_NOARG
#define ALDO_BOUNCE aldo::bounce
#define ALDO_NOEXCEPT noexcept
#else
#include <stdbool.h>
#define ALDO_NOARG void
#define ALDO_BOUNCE struct bounce
#define ALDO_NOEXCEPT
#endif

struct bounce {
    SDL_Point bounds, pos, velocity;
    int dim;
};

bool imgui_init(SDL_Window *win, SDL_Renderer *ren) ALDO_NOEXCEPT;
void imgui_handle_input(SDL_Event *ev) ALDO_NOEXCEPT;
void imgui_render(const ALDO_BOUNCE *bouncer, bool *show_demo,
                  SDL_Window *win, SDL_Renderer *ren) ALDO_NOEXCEPT;
void imgui_cleanup(ALDO_NOARG) ALDO_NOEXCEPT;

#ifdef __cplusplus
}
}
#endif
#undef ALDO_NOEXCEPT
#undef ALDO_BOUNCE
#undef ALDO_NOARG

#endif
