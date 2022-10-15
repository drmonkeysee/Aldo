//
//  uiimgui.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/6/22.
//

#include "uiimgui.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

void aldo::imgui_handle_input(SDL_Event* ev) noexcept
{
    ImGui_ImplSDL2_ProcessEvent(ev);
}
