//
//  texture.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/12/24.
//

#ifndef Aldo_gui_texture_hpp
#define Aldo_gui_texture_hpp

#include "attr.hpp"
#include "ctrlsignal.h"
#include "emutypes.hpp"
#include "snapshot.h"

#include "imgui.h"
#include <SDL3/SDL.h>

#include <span>

namespace aldo
{

class Emulator;
class MediaRuntime;
class Palette;
using color_span = std::span<const et::byte, AldoPalSize>;
// TODO: use std::mdspan someday when they can be sliced?
using pt_tile = std::span<const et::word, AldoChrTileDim>;
using pt_span = std::span<const et::word[AldoChrTileDim], AldoPtTileCount>;

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

    Sint64 size() const noexcept { return width * height; }

    Uint32* pixels;
    Sint64 height, width;
    int stride;

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
        float w, h;
        SDL_GetTextureSize(tex, &w, &h);
        ImGui::Image(tex, {w * scalex, h * scaley});
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
        // 4:3 SD TV ratio scales the x-axis by about 1.14 from square
        static constexpr auto tvXScale = 8.0f / 7.0f;

        tex.render(sdRatio ? scale * tvXScale : scale, scale);
    }

private:
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

    tex::Texture<SDL_TEXTUREACCESS_STREAMING> tex;
};

class Nametables {
public:
    static constexpr int
        TilePxDim = pt_tile::extent,
        MetatileDim = 2,
        MetatileCount = MetatileDim * MetatileDim,
        AttributePxDim = TilePxDim * MetatileCount;

    enum class DrawMode {
        nametables,
        attributes,
    };

    Nametables(SDL_Point nametableSize, const MediaRuntime& mr);

    SDL_Point nametableSize() const noexcept { return ntSize; }
    SDL_Point nametableExtent() const noexcept { return texSize; };

    void draw(const Emulator& emu, const MediaRuntime& mr) const;
    void render() const noexcept
    {
        if (mode == DrawMode::attributes) {
            atTex.render();
        } else {
            ntTex.render();
        }
    }

    DrawMode mode = DrawMode::nametables;

private:
    static constexpr int AttributeDim = 8;

    using attr_span = std::span<const et::byte, AldoNtAttrCount>;
    using pal_span = std::span<const et::byte[AldoPalSize], AldoPalSize>;
    struct nt_offsets {
        int upperX = 0, upperY = 0, mirrorX = 0, mirrorY = 0;
    };

    void drawNametables(const Emulator& emu) const;
    void drawAttributes(const Emulator& emu, const MediaRuntime& mr) const;
    nt_offsets getOffsets(aldo_ntmirror m) const noexcept;
    static color_span lookupTilePalette(attr_span attrs, int tileCol, int tileRow,
                                        pal_span palettes) noexcept;
    static void drawAttribute(attr_span attrs, int ntIdx, int col, int row,
                              const nt_offsets& offsets, pal_span bg,
                              const Palette& p, SDL_Renderer* ren);
    static void drawMetatile(et::byte attr, int ntIdx, int col, int row,
                             int metaTile, const nt_offsets& offsets,
                             pal_span bg, const Palette& p, SDL_Renderer* ren);

    SDL_Point ntSize, texSize;
    tex::Texture<SDL_TEXTUREACCESS_STREAMING> ntTex;
    tex::Texture<SDL_TEXTUREACCESS_TARGET> atTex;
};

class Sprites {
public:
    enum class Priority {
        all,
        front,
        back,
    };

    explicit Sprites(const aldo::MediaRuntime& mr);

    void draw(const Emulator& emu) const;
    void render() const noexcept
    {
        sprTex.render();
    }

    Priority priority = Priority::all;

private:
    tex::Texture<SDL_TEXTUREACCESS_STREAMING> sprTex;
};

}

#endif
