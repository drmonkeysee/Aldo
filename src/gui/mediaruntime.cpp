//
//  mediaruntime.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/1/22.
//

#include "mediaruntime.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

#include <stdexcept>

namespace
{

auto create_window(const aldo::guiopts& options, SDL_Point windowSize) noexcept
{
   const Uint32 flags = options.hi_dpi ? SDL_WINDOW_ALLOW_HIGHDPI : 0;
   return SDL_CreateWindow("Aldo",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           windowSize.x, windowSize.y,
                           flags);
}

auto create_renderer(const aldo::WindowHandle& hwin) noexcept
{
    static constexpr Uint32 flags = SDL_RENDERER_ACCELERATED
                                    | SDL_RENDERER_PRESENTVSYNC;
    return SDL_CreateRenderer(hwin.raw(), -1, flags);
}

}

//
// Public Interface
//

aldo::SdlLib::SdlLib()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL initialization failure: %s", SDL_GetError());
        throw std::runtime_error{"SDL runtime failure"};
    }
}

aldo::SdlLib::~SdlLib()
{
    SDL_Quit();
}

aldo::WindowHandle::WindowHandle(const aldo::guiopts& options,
                                 SDL_Point windowSize)
: win{create_window(options, windowSize)}
{
    if (!this->win) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL window creation failure: %s", SDL_GetError());
        throw std::runtime_error{"SDL window creation failure"};
    }
}

aldo::RendererHandle::RendererHandle(const aldo::guiopts& options,
                                     const aldo::WindowHandle& hwin)
: ren{create_renderer(hwin)}
{
    if (!this->ren) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL renderer creation failure: %s",
                        SDL_GetError());
        throw std::runtime_error{"SDL renderer creation failure"};
    }
    SDL_RenderSetScale(raw(),
                       options.render_scale_factor,
                       options.render_scale_factor);
    SDL_RendererInfo info;
    SDL_GetRendererInfo(raw(), &info);
    SDL_Log("Render name: %s (%04X)", info.name, info.flags);
}

aldo::DearImGuiLib::DearImGuiLib(const aldo::WindowHandle& hwin,
                                 const aldo::RendererHandle& hren)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(hwin.raw(), hren.raw());
    ImGui_ImplSDLRenderer_Init(hren.raw());
}

aldo::DearImGuiLib::~DearImGuiLib()
{
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}
