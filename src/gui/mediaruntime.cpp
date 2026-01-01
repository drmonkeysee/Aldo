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
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include <stdexcept>
#include <cinttypes>

namespace
{

ALDO_OWN
auto create_window(SDL_Point windowSize, const gui_platform& p)
{
    auto hidpi = p.is_hidpi();
    SDL_Log("HIDPI: %d", hidpi);
    aldo::platform_buffer name{p.appname(), p.free_buffer};
    auto win = SDL_CreateWindow(name ? name.get() : "DisplayNameErr",
                                windowSize.x, windowSize.y,
                                hidpi ? SDL_WINDOW_HIGH_PIXEL_DENSITY : 0);
    if (!win) throw aldo::SdlError{"SDL window creation failure"};
    return win;
}

ALDO_OWN
auto create_renderer(const aldo::mr::win_handle& hwin, const gui_platform& p)
{
    auto props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER,
                           hwin.get());
    SDL_SetNumberProperty(props, SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER,
                          1);
    auto ren = SDL_CreateRendererWithProperties(props);
    SDL_DestroyProperties(props);
    if (!ren) throw aldo::SdlError{"SDL renderer creation failure"};

    auto render_scale_factor = p.render_scale_factor(hwin.get());
    SDL_SetRenderScale(ren, render_scale_factor, render_scale_factor);
    props = SDL_GetRendererProperties(ren);
    auto vsync = SDL_GetNumberProperty(props, SDL_PROP_RENDERER_VSYNC_NUMBER,
                                       0);
    SDL_DestroyProperties(props);
    SDL_Log("Render info: %s (%" PRId64 ") (x%.1f)", SDL_GetRendererName(ren),
            vsync, render_scale_factor);
    return ren;
}

}

//
// MARK: - Public Interface
//

aldo::mr::SdlLib::SdlLib()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
        throw aldo::SdlError{"SDL initialization failure"};
}

aldo::mr::SdlLib::~SdlLib()
{
    SDL_Quit();
}

aldo::mr::DearImGuiLib::DearImGuiLib(const aldo::mr::win_handle& hwin,
                                     const aldo::mr::ren_handle& hren)
{
    if (!IMGUI_CHECKVERSION())
        throw std::runtime_error{"DearImGui initialization failure"};

    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForSDLRenderer(hwin.get(), hren.get());
    ImGui_ImplSDLRenderer3_Init(hren.get());

    ImGui::StyleColorsDark();
}

aldo::mr::DearImGuiLib::~DearImGuiLib()
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

aldo::MediaRuntime::MediaRuntime(SDL_Point windowSize, const gui_platform& p)
try : p{p}, hwin{create_window(windowSize, p)}, hren{create_renderer(hwin, p)},
        imgui{hwin, hren} {}
catch (...) {
    InitStatus = ALDO_UI_ERR_LIBINIT;
}
