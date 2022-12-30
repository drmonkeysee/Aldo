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
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <SDL2/SDL.h>

#include <utility>

aldo::RenderFrame::RenderFrame(const aldo::MediaRuntime& r,
                               aldo::RunTimer t) noexcept
: r{r}, t{std::move(t)}
{
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

aldo::RenderFrame::~RenderFrame()
{
    const auto ren = r.renderer();
    ImGui::Render();
    SDL_SetRenderDrawColor(ren, aldo::colors::ScreenFill,
                           aldo::colors::ScreenFill, aldo::colors::ScreenFill,
                           SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    // NOTE: record render timing here, otherwise we're just measuring VSYNC
    t.record();
    SDL_RenderPresent(ren);
}
