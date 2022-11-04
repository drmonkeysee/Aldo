//
//  mediaruntime.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/3/22.
//

#ifndef Aldo_gui_mediaruntime_hpp
#define Aldo_gui_mediaruntime_hpp

#include "guiplatform.h"

#include <SDL2/SDL.h>

#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace aldo
{

template<auto f>
using func_deleter = std::integral_constant<std::decay_t<decltype(f)>, f>;
using win_handle
    = std::unique_ptr<SDL_Window, func_deleter<SDL_DestroyWindow>>;
using ren_handle
    = std::unique_ptr<SDL_Renderer, func_deleter<SDL_DestroyRenderer>>;
using tex_handle
    = std::unique_ptr<SDL_Texture, func_deleter<SDL_DestroyTexture>>;

class SdlError final : public std::runtime_error {
public:
    SdlError(std::string_view message);
};

class SdlLib final {
public:
    SdlLib();
    SdlLib(const SdlLib&) = delete;
    SdlLib& operator=(const SdlLib&) = delete;
    SdlLib(SdlLib&&) = delete;
    SdlLib& operator=(SdlLib&&) = delete;
    ~SdlLib();
};

class DearImGuiLib final {
public:
    DearImGuiLib(const win_handle& hwin, const ren_handle& hren);
    DearImGuiLib(const DearImGuiLib&) = delete;
    DearImGuiLib& operator=(const DearImGuiLib&) = delete;
    DearImGuiLib(DearImGuiLib&&) = delete;
    DearImGuiLib& operator=(DearImGuiLib&&) = delete;
    ~DearImGuiLib();
};

class MediaRuntime final {
public:
    static int initStatus() noexcept { return InitStatus; }

    MediaRuntime(SDL_Point windowSize, SDL_Point screenResolution,
                 const gui_platform& p) noexcept;
    MediaRuntime(const MediaRuntime&) = delete;
    MediaRuntime& operator=(const MediaRuntime&) = delete;
    MediaRuntime(MediaRuntime&&) = delete;
    MediaRuntime& operator=(MediaRuntime&&) = delete;
    ~MediaRuntime() = default;

    SDL_Window* window() const noexcept { return hwin.get(); }
    SDL_Renderer* renderer() const noexcept { return hren.get(); }
    SDL_Texture* bouncerTexture() const noexcept { return htex.get(); }

private:
    inline static int InitStatus = 0;

    SdlLib sdl;
    win_handle hwin;
    ren_handle hren;
    tex_handle htex;
    DearImGuiLib imgui;
};

}

#endif
