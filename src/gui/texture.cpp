//
//  texture.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/12/24.
//

#include "texture.hpp"

#include "error.hpp"
#include "mediaruntime.hpp"
#include "palette.hpp"
#include "viewstate.hpp"

#include <concepts>

static_assert(std::same_as<Uint32, ImU32>,
              "SDL u32 type does not match ImGui u32 type");

//
// MARK: - Public Interface
//

aldo::BouncerScreen::BouncerScreen(SDL_Point resolution,
                                   const aldo::MediaRuntime& mr)
: tex{resolution, mr.renderer()} {}

void aldo::BouncerScreen::draw(const aldo::viewstate& vs,
                               const aldo::MediaRuntime& mr) const noexcept
{
    auto ren = mr.renderer();
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

aldo::PatternTable::PatternTable(const aldo::MediaRuntime& mr)
: tex{{TextureDim, TextureDim}, mr.renderer()} {}

void aldo::PatternTable
::draw(const aldo::et::word table[ALDO_PT_TILE_COUNT][ALDO_CHR_TILE_DIM],
       const aldo::et::byte colors[ALDO_PAL_SIZE], const Palette& p,
       aldo::palette::emphasis em) const
{
    auto data = tex.lock();
    for (auto tblRow = 0; tblRow < TableDim; ++tblRow) {
        for (auto tblCol = 0; tblCol < TableDim; ++tblCol) {
            const auto* tile = table[tblCol + (tblRow * TableDim)];
            for (auto tileRow = 0; tileRow < ALDO_CHR_TILE_DIM; ++tileRow) {
                auto rowOffset = (tblCol * ALDO_CHR_TILE_DIM)
                                    + ((tileRow + (tblRow * ALDO_CHR_TILE_DIM))
                                       * data.stride);
                drawTableRow(tile[tileRow], colors, rowOffset, p, data, em);
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
aldo::PatternTable::drawTableRow(aldo::et::word pixels,
                                 const aldo::et::byte colors[ALDO_PAL_SIZE],
                                 int texOffset, const aldo::Palette& p,
                                 const aldo::texture::TextureData& data,
                                 aldo::palette::emphasis em)
{
    for (auto px = 0; px < ALDO_CHR_TILE_DIM; ++px) {
        auto pidx = ALDO_CHR_TILE_STRIDE - ((px + 1) * 2);
        assert(0 <= pidx);
        auto texel = (pixels & (0x3 << pidx)) >> pidx;
        assert(texel < ALDO_PAL_SIZE);
        auto texidx = px + texOffset;
        assert(texidx < TextureDim * TextureDim);
        data.pixels[texidx] = p.getColor(colors[texel], em);
    }
}
