//
//  mediaruntime.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/1/22.
//

#ifndef Aldo_gui_mediaruntime_hpp
#define Aldo_gui_mediaruntime_hpp

#include "options.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <SDL2/SDL.h>

#include <memory>
#include <stdexcept>
#include <type_traits>

namespace aldo
{

template<auto d>
using sdl_deleter = std::integral_constant<std::decay_t<decltype(d)>, d>;

class SdlLib final {
public:
    SdlLib()
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                            "SDL initialization failure: %s", SDL_GetError());
            throw std::runtime_error{"SDL runtime failure"};
        }
    }

    SdlLib(const SdlLib&) = delete;
    SdlLib& operator=(const SdlLib&) = delete;

    ~SdlLib() { SDL_Quit(); }
};

class WindowHandle final {
public:
    WindowHandle(const aldo::aldo_guiopts& options, SDL_Point windowSize)
    : win{createWindow(options, windowSize)}
    {
        if (!this->win) {
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                            "SDL window creation failure: %s", SDL_GetError());
            throw std::runtime_error{"SDL window creation failure"};
        }
    }

    SDL_Window* raw() const noexcept { return win.get(); }

private:
    static SDL_Window* createWindow(const aldo::aldo_guiopts& options,
                                     SDL_Point windowSize) noexcept
   {
       const Uint32 flags = options.hi_dpi ? SDL_WINDOW_ALLOW_HIGHDPI : 0;
       return SDL_CreateWindow("Aldo",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               windowSize.x, windowSize.y,
                               flags);
   }

    std::unique_ptr<SDL_Window, aldo::sdl_deleter<SDL_DestroyWindow>> win;
};

class RendererHandle final {
public:
    RendererHandle(const aldo::aldo_guiopts& options,
                   const aldo::WindowHandle& hwin)
    : ren{createRenderer(hwin)}
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
        SDL_Log("Render name: %s (%X)", info.name, info.flags);
    }

    SDL_Renderer* raw() const noexcept { return ren.get(); }

private:
    static SDL_Renderer*
    createRenderer(const aldo::WindowHandle& hwin) noexcept
    {
        const Uint32 flags = SDL_RENDERER_ACCELERATED
                                | SDL_RENDERER_PRESENTVSYNC;
        return SDL_CreateRenderer(hwin.raw(), -1, flags);
    }

    std::unique_ptr<SDL_Renderer, aldo::sdl_deleter<SDL_DestroyRenderer>> ren;
};

class DearImGuiLib final {
public:
    DearImGuiLib(const aldo::WindowHandle& hwin,
                 const aldo::RendererHandle& hren)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForSDLRenderer(hwin.raw(), hren.raw());
        ImGui_ImplSDLRenderer_Init(hren.raw());
    }

    DearImGuiLib(const DearImGuiLib&) = delete;
    DearImGuiLib& operator=(const DearImGuiLib&) = delete;

    ~DearImGuiLib()
    {
        ImGui_ImplSDLRenderer_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }
};

class MediaRuntime final {
public:
    MediaRuntime(const aldo::aldo_guiopts& options, SDL_Point windowSize)
    : win{options, windowSize}, ren{options, win}, imgui{win, ren} { }

    SDL_Window* window() const noexcept { return win.raw(); }
    SDL_Renderer* renderer() const noexcept { return ren.raw(); }

private:
    aldo::SdlLib sdl;
    aldo::WindowHandle win;
    aldo::RendererHandle ren;
    aldo::DearImGuiLib imgui;
};

}

#endif
