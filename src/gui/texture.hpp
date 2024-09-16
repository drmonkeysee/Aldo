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

namespace aldo
{

struct viewstate;
class Palette;
template<SDL_TextureAccess> class Texture;

namespace texture
{

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
    int stride;

private:
    friend Texture<SDL_TEXTUREACCESS_STREAMING>;

    TextureData(SDL_Texture& tex) noexcept;

    SDL_Texture& tex;
};

}

template<SDL_TextureAccess Access>
class Texture {
public:
    Texture(SDL_Point size, SDL_Renderer* ren)
    : tex{texture::create(size, Access, ren)} {}
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = delete;
    Texture& operator=(Texture&&) = delete;
    ~Texture() { SDL_DestroyTexture(tex); }

    texture::TextureTarget asTarget(SDL_Renderer *ren) const noexcept
    requires (Access == SDL_TEXTUREACCESS_TARGET) { return {*ren, *tex}; }

    texture::TextureData lock() const noexcept
    requires (Access == SDL_TEXTUREACCESS_STREAMING) { return *tex; }

    void render(float scale = 1.0f) const noexcept
    {
        int w, h;
        SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
        ImGui::Image(tex, {w * scale, h * scale});
    }

private:
    SDL_Texture* tex;
};

class BouncerScreen {
public:
    BouncerScreen(SDL_Point resolution, SDL_Renderer* ren);

    void draw(const viewstate& vs, SDL_Renderer* ren) const noexcept;
    void render() const noexcept { tex.render(); }

private:
    Texture<SDL_TEXTUREACCESS_TARGET> tex;
};

class PatternTable {
public:
    PatternTable(SDL_Renderer* ren);

    void draw(const et::word table[CHR_PAT_TILES][CHR_TILE_DIM],
              const Palette& palette) const;
    void render() const noexcept { tex.render(2.0); }

private:
    static constexpr int TableDim = 16, TextureDim = 128;
    static_assert(TableDim * TableDim == CHR_PAT_TILES,
                  "Table size does not match tile count");
    static_assert(TextureDim == TableDim * CHR_TILE_DIM,
                  "Texture size does not match tile pixel count");

    static void drawTableRow(et::word tileRow, int texOffset, const Palette& p,
                             const texture::TextureData& data);

    Texture<SDL_TEXTUREACCESS_STREAMING> tex;
};

}

#endif
