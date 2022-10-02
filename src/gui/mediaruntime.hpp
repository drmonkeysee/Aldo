//
//  mediaruntime.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/1/22.
//

#ifndef Aldo_gui_mediaruntime_hpp
#define Aldo_gui_mediaruntime_hpp

#include "options.h"

#include <SDL2/SDL.h>

#include <memory>
#include <type_traits>

namespace aldo
{

template<auto d>
using sdl_deleter = std::integral_constant<std::decay_t<decltype(d)>, d>;

class SdlLib final {
public:
    SdlLib();
    SdlLib(const SdlLib&) = delete;
    SdlLib& operator=(const SdlLib&) = delete;
    ~SdlLib();
};

class WindowHandle final {
public:
    WindowHandle(const aldo::aldo_guiopts& options, SDL_Point windowSize);

    SDL_Window* raw() const noexcept { return win.get(); }

private:
    std::unique_ptr<SDL_Window, aldo::sdl_deleter<SDL_DestroyWindow>> win;
};

class RendererHandle final {
public:
    RendererHandle(const aldo::aldo_guiopts& options,
                   const aldo::WindowHandle& hwin);

    SDL_Renderer* raw() const noexcept { return ren.get(); }

private:
    std::unique_ptr<SDL_Renderer, aldo::sdl_deleter<SDL_DestroyRenderer>> ren;
};

class DearImGuiLib final {
public:
    DearImGuiLib(const aldo::WindowHandle& hwin,
                 const aldo::RendererHandle& hren);
    DearImGuiLib(const DearImGuiLib&) = delete;
    DearImGuiLib& operator=(const DearImGuiLib&) = delete;
    ~DearImGuiLib();
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
