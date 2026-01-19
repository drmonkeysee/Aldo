//
//  render.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/4/22.
//

#include "render.hpp"

#include "mediaruntime.hpp"
#include "style.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>

#include <utility>

aldo::RenderFrame::RenderFrame(const aldo::MediaRuntime& r,
                               aldo::RunTimer t) noexcept
: r{r}, t{std::move(t)}
{
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

aldo::RenderFrame::~RenderFrame()
{
    ImGui::Render();
    auto ren = r.renderer();
    SDL_SetRenderDrawColor(ren, aldo::colors::ScreenFill,
                           aldo::colors::ScreenFill, aldo::colors::ScreenFill,
                           SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), ren);
    // record render timing here, otherwise we're just measuring VSYNC
    t.record();
    SDL_RenderPresent(ren);
}
