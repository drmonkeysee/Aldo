//
//  mediaruntime.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/3/22.
//

#ifndef Aldo_gui_mediaruntime_hpp
#define Aldo_gui_mediaruntime_hpp

#include "attr.hpp"
#include "handle.hpp"

#include <SDL2/SDL.h>

struct gui_platform;

namespace aldo
{

using win_handle = handle<SDL_Window, SDL_DestroyWindow>;
using ren_handle = handle<SDL_Renderer, SDL_DestroyRenderer>;
using tex_handle = handle<SDL_Texture, SDL_DestroyTexture>;

class ALDO_SIDEFX SdlLib {
public:
    SdlLib();
    SdlLib(const SdlLib&) = delete;
    SdlLib& operator=(const SdlLib&) = delete;
    SdlLib(SdlLib&&) = delete;
    SdlLib& operator=(SdlLib&&) = delete;
    ~SdlLib();
};

class ALDO_SIDEFX DearImGuiLib {
public:
    DearImGuiLib(const win_handle& hwin, const ren_handle& hren);
    DearImGuiLib(const DearImGuiLib&) = delete;
    DearImGuiLib& operator=(const DearImGuiLib&) = delete;
    DearImGuiLib(DearImGuiLib&&) = delete;
    DearImGuiLib& operator=(DearImGuiLib&&) = delete;
    ~DearImGuiLib();
};

class ALDO_SIDEFX MediaRuntime {
public:
    [[nodiscard("check error")]]
    static int initStatus() noexcept { return InitStatus; }

    MediaRuntime(SDL_Point windowSize, SDL_Point screenResolution,
                 const gui_platform& p);

    SDL_Window* window() const noexcept { return hwin.get(); }
    SDL_Renderer* renderer() const noexcept { return hren.get(); }
    SDL_Texture* bouncerTexture() const noexcept { return htex.get(); }

private:
    inline static int InitStatus;

    SdlLib sdl;
    win_handle hwin;
    ren_handle hren;
    tex_handle htex;
    DearImGuiLib imgui;
};

}

#endif
