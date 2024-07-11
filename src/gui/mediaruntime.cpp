//
//  mediaruntime.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/3/22.
//

#include "mediaruntime.hpp"

#include "attr.hpp"
#include "error.hpp"
#include "guiplatform.h"
#include "ui.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include <memory>

namespace
{

ALDO_OWN
auto create_window(SDL_Point windowSize, const gui_platform& p)
{
    const auto hidpi = p.is_hidpi();
    SDL_Log("HIDPI: %d", hidpi);
    const aldo::platform_buffer name{p.appname(), p.free_buffer};
    const auto win = SDL_CreateWindow(name ? name.get() : "DisplayNameErr",
                                      SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                                      windowSize.x, windowSize.y,
                                      hidpi ? SDL_WINDOW_ALLOW_HIGHDPI : 0);
    if (!win) throw aldo::SdlError{"SDL window creation failure"};
    return win;
}

ALDO_OWN
auto create_renderer(const aldo::win_handle& hwin, const gui_platform& p)
{
    const auto ren = SDL_CreateRenderer(hwin.get(), -1,
                                        SDL_RENDERER_ACCELERATED
                                        | SDL_RENDERER_PRESENTVSYNC
                                        | SDL_RENDERER_TARGETTEXTURE);
    if (!ren) throw aldo::SdlError{"SDL renderer creation failure"};

    const auto render_scale_factor = p.render_scale_factor(hwin.get());
    SDL_RenderSetScale(ren, render_scale_factor, render_scale_factor);
    SDL_RendererInfo info;
    SDL_GetRendererInfo(ren, &info);
    SDL_Log("Render info: %s (%04X) (x%.1f)", info.name, info.flags,
            render_scale_factor);
    return ren;
}

ALDO_OWN
auto create_bouncer_texture(SDL_Point screenResolution,
                            const aldo::ren_handle& hren)
{
    const auto tex = SDL_CreateTexture(hren.get(), SDL_PIXELFORMAT_RGBA32,
                                       SDL_TEXTUREACCESS_TARGET,
                                       screenResolution.x, screenResolution.y);
    if (!tex) throw aldo::SdlError{"Bouncer texture creation failure"};
    return tex;
}

}

//
// MARK: - Public Interface
//

aldo::SdlLib::SdlLib()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        throw aldo::SdlError{"SDL initialization failure"};
}

aldo::SdlLib::~SdlLib()
{
    SDL_Quit();
}

aldo::DearImGuiLib::DearImGuiLib(const aldo::win_handle& hwin,
                                 const aldo::ren_handle& hren)
{
    if (!IMGUI_CHECKVERSION())
        throw std::runtime_error{"DearImGui initialization failure"};

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(hwin.get(), hren.get());
    ImGui_ImplSDLRenderer2_Init(hren.get());

    ImGui::StyleColorsDark();
}

aldo::DearImGuiLib::~DearImGuiLib()
{
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

aldo::MediaRuntime::MediaRuntime(SDL_Point windowSize,
                                 SDL_Point screenResolution,
                                 const gui_platform& p)
try : hwin{create_window(windowSize, p)},
        hren{create_renderer(hwin, p)},
        htex{create_bouncer_texture(screenResolution, hren)},
        imgui{hwin, hren} {}
catch (...) {
    InitStatus = UI_ERR_LIBINIT;
}
