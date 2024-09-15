//
//  texture.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/12/24.
//

#include "texture.hpp"

#include "error.hpp"
#include "viewstate.hpp"

#include <array>
#include <concepts>

static_assert(std::same_as<Uint32, ImU32>,
              "SDL u32 type does not match ImGui u32 type");

//
// MARK: - Public Interface
//

aldo::BouncerScreen::BouncerScreen(SDL_Point resolution, SDL_Renderer* ren)
: tex{resolution, ren} {}

void aldo::BouncerScreen::draw(const aldo::viewstate& vs,
                               SDL_Renderer* ren) const noexcept
{
    auto target = tex.asTarget(ren);

    SDL_SetRenderDrawColor(ren, 0x0, 0xff, 0xff, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);

    SDL_SetRenderDrawColor(ren, 0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(ren, 30, 7, 50, 200);

    SDL_SetRenderDrawColor(ren, 0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);

    auto& bouncer = vs.bouncer;
    auto fulldim = bouncer.halfdim * 2;
    SDL_Rect pos{
        bouncer.pos.x - bouncer.halfdim,
        bouncer.pos.y - bouncer.halfdim,
        fulldim,
        fulldim,
    };
    SDL_RenderFillRect(ren, &pos);
}

aldo::PatternTable::PatternTable(SDL_Renderer* ren)
: tex{{TextureDim, TextureDim}, ren} {}

void aldo::PatternTable
::draw(const aldo::et::word table[CHR_PAT_TILES][CHR_TILE_DIM],
       const Palette& palette) const
{
    auto texData = tex.lock();
    for (auto tblRow = 0; tblRow < TableDim; ++tblRow) {
        for (auto tblCol = 0; tblCol < TableDim; ++tblCol) {
            const auto* tile = table[tblCol + (tblRow * TableDim)];
            for (auto tileRow = 0; tileRow < CHR_TILE_DIM; ++tileRow) {
                auto texy = (tileRow + (tblRow * CHR_TILE_DIM))
                            * texData.stride;
                drawTileRow(tile[tileRow], tblCol, texy, palette, texData);
            }
        }
    }
}

//
// MARK: - Internal Interface
//

SDL_Texture* aldo::texture::create(SDL_Point size, SDL_TextureAccess access,
                                   SDL_Renderer* ren)
{
    auto tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, access, size.x,
                                 size.y);
    if (!tex) throw aldo::SdlError{"SDL texture creation failure"};
    return tex;
}

aldo::texture::TextureData::TextureData(SDL_Texture& tex) noexcept : tex{tex}
{
    void* data;
    int pitch;
    SDL_LockTexture(&tex, nullptr, &data, &pitch);
    pixels = static_cast<Uint32*>(data);
    stride = pitch / static_cast<int>(sizeof *pixels);
}

//
// MARK: - Private Interface
//

void
aldo::PatternTable::drawTileRow(aldo::et::word tileRow, int tblx, int texy,
                                const aldo::Palette& palette,
                                const aldo::texture::TextureData& texData)
{
    // TODO: replace this with real palette lookup
    static constexpr std::array<aldo::palette::sz, 4> colors = {
        0x1d, 0x0, 0x10, 0x20,
    };

    for (auto px = 0; px < CHR_TILE_DIM; ++px) {
        auto pidx = CHR_TILE_STRIDE - ((px + 1) * 2);
        assert(0 <= pidx);
        decltype(colors)::size_type pixel = (tileRow & (0x3 << pidx)) >> pidx;
        auto
            texx = px + (tblx * CHR_TILE_DIM),
            texidx = texx + texy;
        assert(texidx < TextureDim * TextureDim);
        texData.pixels[texidx] = palette.getColor(colors[pixel]);
    }
}
