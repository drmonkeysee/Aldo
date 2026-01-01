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

#include <SDL3/SDL.h>

#include <functional>
#include <utility>

struct gui_platform;

namespace aldo
{

namespace mr
{

using win_handle = handle<SDL_Window, SDL_DestroyWindow>;
using ren_handle = handle<SDL_Renderer, SDL_DestroyRenderer>;

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

}

class ALDO_SIDEFX SdlProperties {
public:
    static SdlProperties renderer(SDL_Renderer* ren) noexcept
    {
        return SdlProperties{SDL_GetRendererProperties(ren)};
    }
    static SdlProperties texture(SDL_Texture* tex) noexcept
    {
        return SdlProperties{SDL_GetTextureProperties(tex)};
    }

    SdlProperties() noexcept : SdlProperties{SDL_CreateProperties()}
    {
        cleanup = [this] noexcept { SDL_DestroyProperties(this->props); };
    }
    explicit SdlProperties(SDL_PropertiesID pid) noexcept : props{pid} {}
    SdlProperties(const SdlProperties&) = delete;
    SdlProperties& operator=(const SdlProperties&) = delete;
    SdlProperties(SdlProperties&&) noexcept = default;
    SdlProperties& operator=(SdlProperties&& that) noexcept
    {
        if (this == &that) return *this;

        swap(that);
        return *this;
    }
    ~SdlProperties() { cleanup(); }

    SDL_PropertiesID get() const noexcept { return props; }

    Sint64 number(const char* name, Sint64 defaultVal = 0) const noexcept
    {
        return SDL_GetNumberProperty(props, name, defaultVal);
    }
    void setNumber(const char* name, Sint64 val) noexcept
    {
        SDL_SetNumberProperty(props, name, val);
    }

    void setPointer(const char* name, void* val) noexcept
    {
        SDL_SetPointerProperty(props, name, val);
    }

    void swap(SdlProperties& that) noexcept
    {
        using std::swap;

        swap(props, that.props);
        swap(cleanup, that.cleanup);
    }

private:
    using dtor = std::function<void()>;

    SDL_PropertiesID props;
    dtor cleanup = [] static noexcept {};
};

class ALDO_SIDEFX MediaRuntime {
public:
    [[nodiscard("check error")]]
    static int initStatus() noexcept { return InitStatus; }

    MediaRuntime(SDL_Point windowSize, const gui_platform& p);
    MediaRuntime(SDL_Point, gui_platform&&) = delete;

    const gui_platform& platform() const noexcept { return p; }
    SDL_Window* window() const noexcept { return hwin.get(); }
    SDL_Renderer* renderer() const noexcept { return hren.get(); }

private:
    inline static int InitStatus;

    const gui_platform& p;
    mr::SdlLib sdl;
    mr::win_handle hwin;
    mr::ren_handle hren;
    mr::DearImGuiLib imgui;
};

inline void swap(SdlProperties& a, SdlProperties& b) noexcept
{
    a.swap(b);
}

}

#endif
