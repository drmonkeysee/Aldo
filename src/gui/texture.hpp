//
//  texture.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/12/24.
//

#ifndef Aldo_gui_texture_hpp
#define Aldo_gui_texture_hpp

#include "attr.hpp"
#include "emutypes.hpp"
#include "snapshot.h"

#include "imgui.h"
#include <SDL2/SDL.h>

#include <span>

namespace aldo
{

class Emulator;
class MediaRuntime;
class Palette;
using color_span = std::span<const et::byte, ALDO_PAL_SIZE>;
// TODO: use std::mdspan someday when they can be sliced?
using pt_tile = std::span<const et::word, ALDO_CHR_TILE_DIM>;
using pt_span = std::span<const et::word[ALDO_CHR_TILE_DIM],
                            ALDO_PT_TILE_COUNT>;

namespace tex
{

template<SDL_TextureAccess> class Texture;

ALDO_OWN
SDL_Texture* create(SDL_Point size, SDL_TextureAccess access,
                    SDL_Renderer* ren);

class ALDO_SIDEFX TextureTarget {
public:
    TextureTarget(const TextureTarget&) = delete;
    TextureTarget& operator=(const TextureTarget&) = delete;
    TextureTarget(TextureTarget&&) = delete;
    TextureTarget& operator=(TextureTarget&&) = delete;
    ~TextureTarget() { SDL_SetRenderTarget(&ren, nullptr); }

private:
    friend Texture<SDL_TEXTUREACCESS_TARGET>;

    TextureTarget(SDL_Renderer& ren, SDL_Texture& tex) noexcept : ren{ren}
    {
        SDL_SetRenderTarget(&ren, &tex);
    }

    SDL_Renderer& ren;
};

class ALDO_SIDEFX TextureData {
public:
    TextureData(const TextureData&) = delete;
    TextureData& operator=(const TextureData&) = delete;
    TextureData(TextureData&&) = delete;
    TextureData& operator=(TextureData&&) = delete;
    ~TextureData() { SDL_UnlockTexture(&tex); }

    Uint32* pixels;
    int height, stride, width;

private:
    friend Texture<SDL_TEXTUREACCESS_STREAMING>;

    TextureData(SDL_Texture& tex) noexcept;

    SDL_Texture& tex;
};

template<SDL_TextureAccess Access>
class Texture {
public:
    Texture(SDL_Point size, SDL_Renderer* ren)
    : tex{create(size, Access, ren)} {}
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = delete;
    Texture& operator=(Texture&&) = delete;
    ~Texture() { SDL_DestroyTexture(tex); }

    TextureTarget asTarget(SDL_Renderer* ren) const noexcept
    requires (Access == SDL_TEXTUREACCESS_TARGET) { return {*ren, *tex}; }

    TextureData lock() const noexcept
    requires (Access == SDL_TEXTUREACCESS_STREAMING) { return *tex; }

    void render(float scale = 1.0f) const noexcept
    {
        render(scale, scale);
    }

    void render(float scalex, float scaley) const noexcept
    {
        int w, h;
        SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
        ImGui::Image(tex, {
            static_cast<float>(w) * scalex, static_cast<float>(h) * scaley,
        });
    }

private:
    SDL_Texture* tex;
};

}

class VideoScreen {
public:
    VideoScreen(SDL_Point resolution, const MediaRuntime& mr);

    void draw(const et::byte* vbuf, const Palette& p) const;
    void render(float scale, bool sdRatio) const noexcept
    {
        tex.render(sdRatio ? scale * TvXScale : scale, scale);
    }

private:
    // NOTE: 4:3 SD TV ratio scales the x-axis by about 1.14 from square
    static constexpr float TvXScale = 8.0f / 7.0f;

    tex::Texture<SDL_TEXTUREACCESS_STREAMING> tex;
};

class PatternTable {
public:
    PatternTable(const MediaRuntime& mr);

    void draw(pt_span table, color_span colors, const Palette& p) const;
    void render() const noexcept { tex.render(2.0); }

private:
    static constexpr int TableDim = 16, TextureDim = 128;
    static_assert(TableDim * TableDim == pt_span::extent,
                  "Table size does not match tile count");
    static_assert(TextureDim == TableDim * pt_tile::extent,
                  "Texture size does not match tile pixel count");

    static void drawTileRow(et::word row, color_span colors, int texOffset,
                            const Palette& p, const tex::TextureData& data);

    tex::Texture<SDL_TEXTUREACCESS_STREAMING> tex;
};

class Nametables {
public:
    Nametables(SDL_Point nametableSize, const MediaRuntime& mr);

    SDL_Point nametableSize() const noexcept { return ntSize; }

    void drawNametables(const Emulator& emu, const MediaRuntime& mr) const;
    void drawAttributes(const Emulator& emu, const MediaRuntime& mr) const;
    void render() const noexcept { tex.render(); }

private:
    static constexpr int
        LayoutDim = ALDO_NT_COUNT, NtDisplayCount = LayoutDim * LayoutDim;

    SDL_Point ntSize;
    tex::Texture<SDL_TEXTUREACCESS_TARGET> tex;
};

}

#endif
