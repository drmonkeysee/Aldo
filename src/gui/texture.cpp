//
//  texture.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/12/24.
//

#include "texture.hpp"

#include "error.hpp"
#include "viewstate.hpp"

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

void
aldo::PatternTable::draw(const aldo::et::word table[CHR_PAT_TILES][CHR_TILE_DIM],
                         const Palette& palette) const noexcept
{
    static constexpr std::array colors = {0x1d, 0x0, 0x10, 0x20};

    auto texData = tex.lock();
    for (auto tblRow = 0; tblRow < TableDim; ++tblRow) {
        for (auto tblCol = 0; tblCol < TableDim; ++tblCol) {
            const auto* tile = table[tblCol + (tblRow * TableDim)];
            for (auto tileRow = 0; tileRow < CHR_TILE_DIM; ++tileRow) {
                auto pixels = tile[tileRow];
                for (auto px = 0; px < CHR_TILE_DIM; ++px) {
                    auto
                        pidx = CHR_TILE_STRIDE - ((px + 1) * 2),
                        pixel = (pixels & (0x3 << pidx)) >> pidx;
                    assert(0 <= pidx);
                    auto
                        texx = px + (tblCol * CHR_TILE_DIM),
                        texy = (tileRow + (tblRow * CHR_TILE_DIM)) * texData.stride,
                        texidx = texx + texy;
                    assert(texidx < TextureDim * TextureDim);
                    aldo::palette::sz palidx = static_cast<aldo::palette::sz>(colors[static_cast<decltype(colors)::size_type>(pixel)]);
                    texData.pixels[texidx] = palette.getColor(palidx);
                }
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
