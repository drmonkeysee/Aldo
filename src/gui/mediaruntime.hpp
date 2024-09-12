//
//  mediaruntime.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 11/3/22.
//

#ifndef Aldo_gui_mediaruntime_hpp
#define Aldo_gui_mediaruntime_hpp

#include "attr.hpp"
#include "error.hpp"
#include "handle.hpp"

#include <SDL2/SDL.h>

#include <concepts>

struct gui_platform;

namespace aldo
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

// TODO: template parameterize with access flags for interface selection
class ALDO_OWN Texture {
public:
    class TextureData;

    Texture(SDL_Point size, SDL_TextureAccess access, const ren_handle& hren);
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = delete;
    Texture& operator=(Texture&&) = delete;
    ~Texture();

    void draw(SDL_Renderer* ren, std::invocable<SDL_Renderer*> auto f) const
    {
        if (SDL_SetRenderTarget(ren, tex) < 0) {
            throw SdlError{"Texture is not a render target"};
        }
        f(ren);
        SDL_SetRenderTarget(ren, nullptr);
    }

    void render(float scale = 1.0f) const noexcept;

    TextureData lock() const { return *tex; }

    class ALDO_SIDEFX TextureData {
    public:
        TextureData(const TextureData&) = delete;
        TextureData& operator=(const TextureData&) = delete;
        TextureData(TextureData&&) = delete;
        TextureData& operator=(TextureData&&) = delete;
        ~TextureData();

        Uint32* pixels;
        int stride;

    private:
        friend Texture;

        TextureData(SDL_Texture& tex);

        SDL_Texture& tex;
    };

private:
    SDL_Texture* tex;
};

class ALDO_SIDEFX MediaRuntime {
public:
    [[nodiscard("check error")]]
    static int initStatus() noexcept { return InitStatus; }

    MediaRuntime(SDL_Point windowSize, SDL_Point screenResolution,
                 const gui_platform& p);

    SDL_Window* window() const noexcept { return hwin.get(); }
    SDL_Renderer* renderer() const noexcept { return hren.get(); }
    const Texture& bouncerScreen() const noexcept { return bouncer; }
    const Texture& patternTableLeft() const noexcept { return patternLeft; }
    const Texture& patternTableRight() const noexcept { return patternRight; }

private:
    inline static int InitStatus;

    SdlLib sdl;
    win_handle hwin;
    ren_handle hren;
    Texture bouncer, patternLeft, patternRight;
    DearImGuiLib imgui;
};

}

#endif
