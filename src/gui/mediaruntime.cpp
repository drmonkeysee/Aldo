//
//  mediaruntime.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/3/22.
//

#include "mediaruntime.hpp"

#include "ui.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

#include <string>

namespace
{

std::string build_sdl_error(std::string_view message)
{
    std::string errorText{message};
    errorText += ": ";
    errorText += SDL_GetError();
    return errorText;
}

SDL_Window* create_window(SDL_Point windowSize, const gui_platform& p)
{
    const auto hidpi = p.is_hidpi();
    SDL_Log("HIDPI: %d", hidpi);
    const auto name = p.appname();
    const auto win = SDL_CreateWindow(name ? name : "DisplayNameErr",
                                      SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                                      windowSize.x, windowSize.y,
                                      hidpi ? SDL_WINDOW_ALLOW_HIGHDPI : 0);
    if (!win) throw aldo::SdlError{"SDL window creation failure"};
    return win;
}

SDL_Renderer* create_renderer(const aldo::win_handle& hwin,
                              const gui_platform& p)
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

SDL_Texture* create_bouncer_texture(SDL_Point screenResolution,
                                    const aldo::ren_handle& hren)
{
    const auto tex = SDL_CreateTexture(hren.get(), SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_TARGET,
                                       screenResolution.x, screenResolution.y);
    if (!tex) throw aldo::SdlError{"Bouncer texture creation failure"};
    return tex;
}

}

//
// Public Interface
//

aldo::SdlError::SdlError(std::string_view message)
: std::runtime_error{build_sdl_error(message)} {}

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
    ImGui_ImplSDLRenderer_Init(hren.get());

    ImGui::StyleColorsDark();
}

aldo::DearImGuiLib::~DearImGuiLib()
{
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

aldo::MediaRuntime::MediaRuntime(SDL_Point windowSize,
                                 SDL_Point screenResolution,
                                 const gui_platform& p) noexcept
try : hwin{create_window(windowSize, p)},
        hren{create_renderer(hwin, p)},
        htex{create_bouncer_texture(screenResolution, hren)},
        imgui{hwin, hren} {}
catch (const std::exception& ex) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", ex.what());
    InitStatus = UI_ERR_LIBINIT;
}
catch (...) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                    "Unknown GUI initialization error!");
    InitStatus = UI_ERR_UNKNOWN;
}
