//
//  texture.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/12/24.
//

#include "texture.hpp"

#include "emu.hpp"
#include "error.hpp"
#include "mediaruntime.hpp"
#include "palette.hpp"
#include "style.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cassert>

static_assert(std::same_as<Uint32, ImU32>,
              "SDL u32 type does not match ImGui u32 type");

namespace
{

using attr_span = std::span<const aldo::et::byte, ALDO_NT_ATTR_COUNT>;
using nt_span = std::span<const aldo::et::byte, ALDO_NT_TILE_COUNT>;

constexpr auto operator*(SDL_Point p, int n) noexcept
{
    return SDL_Point{p.x * n, p.y * n};
}

auto screen_buffer_length() noexcept
{
    auto p = aldo::Emulator::screenSize();
    return p.x * p.y;
}

}

//
// MARK: - Public Interface
//

aldo::VideoScreen::VideoScreen(SDL_Point resolution,
                               const aldo::MediaRuntime& mr)
: tex{resolution, mr.renderer()} {}

void aldo::VideoScreen::draw(const aldo::et::byte* vbuf,
                             const aldo::Palette& p) const
{
    assert(vbuf != nullptr);

    auto data = tex.lock();
    auto length = data.width * data.height;
    assert(length == screen_buffer_length());
    for (auto i = 0; i < length; ++i) {
        data.pixels[i] = p.getColor(vbuf[i]);
    }
}

aldo::PatternTable::PatternTable(const aldo::MediaRuntime& mr)
: tex{{TextureDim, TextureDim}, mr.renderer()} {}

void aldo::PatternTable::draw(aldo::pt_span table, aldo::color_span colors,
                              const aldo::Palette& p) const
{
    using pt_sz = decltype(table)::size_type;

    auto data = tex.lock();
    for (auto tblRow = 0; tblRow < TableDim; ++tblRow) {
        for (auto tblCol = 0; tblCol < TableDim; ++tblCol) {
            auto tileIdx = static_cast<pt_sz>(tblCol + (tblRow * TableDim));
            aldo::pt_tile tile = table[tileIdx];
            auto tileDim = static_cast<int>(tile.size());
            auto tileRow = 0;
            for (auto row : tile) {
                auto rowOffset = (tblCol * tileDim)
                                    + ((tileRow++ + (tblRow * tileDim))
                                       * data.stride);
                drawTileRow(row, colors, rowOffset, p, data);
            }
        }
    }
}

aldo::Nametables::Nametables(SDL_Point nametableSize,
                             const aldo::MediaRuntime& mr)
: ntSize{nametableSize}, texSize{nametableSize * LayoutDim},
ntTex{texSize, mr.renderer()}, atTex{texSize, mr.renderer()} {}

void aldo::Nametables::draw(const Emulator& emu, const MediaRuntime& mr) const
{
    if (mode == aldo::Nametables::DrawMode::attributes) {
        drawAttributes(emu, mr);
    } else {
        drawNametables(emu, mr);
    }
}

//
// MARK: - Internal Interface
//

SDL_Texture* aldo::tex::create(SDL_Point size, SDL_TextureAccess access,
                               SDL_Renderer* ren)
{
    auto tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, access, size.x,
                                 size.y);
    if (!tex) throw aldo::SdlError{"SDL texture creation failure"};
    return tex;
}

aldo::tex::TextureData::TextureData(SDL_Texture& tex) noexcept : tex{tex}
{
    Uint32 pxfmt;
    SDL_QueryTexture(&tex, &pxfmt, nullptr, &width, &height);
    assert(SDL_PIXELTYPE(pxfmt) == SDL_PIXELTYPE_PACKED32);

    void* data;
    int pitch;
    SDL_LockTexture(&tex, nullptr, &data, &pitch);
    pixels = static_cast<Uint32*>(data);
    stride = pitch / static_cast<int>(sizeof *pixels);
}

//
// MARK: - Private Interface
//

void aldo::PatternTable::drawTileRow(aldo::et::word row,
                                     aldo::color_span colors,
                                     int texOffset, const aldo::Palette& p,
                                     const aldo::tex::TextureData& data)
{
    for (auto px = 0; px < static_cast<int>(pt_tile::extent); ++px) {
        auto pidx = ALDO_CHR_TILE_STRIDE - ((px + 1) * 2);
        assert(0 <= pidx);
        decltype(colors)::size_type texel = (row & (0x3 << pidx)) >> pidx;
        assert(texel < color_span::extent);
        auto texidx = px + texOffset;
        assert(texidx < TextureDim * TextureDim);
        data.pixels[texidx] = p.getColor(colors[texel]);
    }
}

void aldo::Nametables::drawNametables(const aldo::Emulator& emu,
                                      const aldo::MediaRuntime& mr) const
{
    static_assert(aldo::color_span::extent == LayoutDim * LayoutDim,
                  "mismatched nt sizes");

    /*
    auto ren = mr.renderer();
    auto target = ntTex.asTarget(ren);
    auto i = 0;
    aldo::color_span colors = emu.snapshot().video->palettes.bg[0];
    for (auto cidx : colors) {
        auto c = emu.palette().getColor(cidx);
        auto [r, g, b] = aldo::colors::rgb(c);
        SDL_SetRenderDrawColor(ren, static_cast<Uint8>(r),
                               static_cast<Uint8>(g), static_cast<Uint8>(b),
                               SDL_ALPHA_OPAQUE);
        auto
            xOffset = i % 2 == 1 ? ntSize.x : 0,
            yOffset = i > 1 ? ntSize.y : 0;
        SDL_Rect nt{xOffset, yOffset, ntSize.x, ntSize.y};
        SDL_RenderFillRect(ren, &nt);
        ++i;
    }
     */

    const auto* vsp = emu.snapshot().video;
    pt_span chrs = vsp->nt.pt
                    ? vsp->pattern_tables.right
                    : vsp->pattern_tables.left;
    color_span colors = vsp->palettes.bg[0];

    // TODO: copied from drawAttributes
    int ntXOffset = 0, ntYOffset = 0, mirXOffset = 0, mirYOffset = 0;
    switch (vsp->nt.mirror) {
    case ALDO_NTM_HORIZONTAL:
        ntYOffset = ntSize.y;
        mirXOffset = ntSize.x;
        break;
    case ALDO_NTM_VERTICAL:
        ntXOffset = ntSize.x;
        mirYOffset = ntSize.y;
        break;
    default:
        return;
    }

    auto data = ntTex.lock();
    for (auto i = 0; i < ALDO_NT_COUNT; ++i) {
        nt_span tiles = vsp->nt.tables[i].tiles;
        for (auto col = 0; col < ALDO_NT_WIDTH; ++col) {
            for (auto row = 0; row < ALDO_NT_HEIGHT; ++row) {
                auto tile = tiles[static_cast<decltype(tiles)::size_type>(col + (row * ALDO_NT_WIDTH))];
                pt_tile chr = chrs[tile];

                // TODO: look up attribute somewhere

                // TODO: copied from PatternTable
                auto chrDim = static_cast<int>(chr.size());
                auto chrRow = 0;
                for (auto pxRow : chr) {
                    auto texOffset = (col * chrDim)
                    + ((chrRow++ + (row * chrDim))
                       * data.stride);
                    texOffset += (i * ntXOffset);
                    texOffset += (i * ntYOffset * data.stride);

                    for (auto px = 0; px < static_cast<int>(pt_tile::extent); ++px) {
                        auto pidx = ALDO_CHR_TILE_STRIDE - ((px + 1) * 2);
                        assert(0 <= pidx);
                        decltype(colors)::size_type texel = (pxRow & (0x3 << pidx)) >> pidx;
                        assert(texel < color_span::extent);
                        auto texidx = px + texOffset;
                        assert(texidx < texSize.x * texSize.y);
                        data.pixels[texidx] = emu.palette().getColor(colors[texel]);
                    }
                }
            }
        }
    }
    if (mirXOffset > 0) {
        for (auto row = 0; row < texSize.y; ++row) {
            auto origin = data.pixels + (row * data.stride);
            std::copy(origin, origin + ntSize.x, origin + mirXOffset);
        }
    }
    if (mirYOffset > 0) {
        for (auto row = 0; row < ntSize.y; ++row) {
            auto origin = data.pixels + (row * data.stride);
            std::copy(origin, origin + texSize.x, origin + (mirYOffset * data.stride));
        }
    }
}

void aldo::Nametables::drawAttributes(const aldo::Emulator& emu,
                                      const aldo::MediaRuntime& mr) const
{
    static constexpr auto
        metatileCount = ALDO_METATILE_DIM * ALDO_METATILE_DIM,
        metatileStride = ALDO_METATILE_DIM * TileDim,
        attrDim = 8;

    const auto* vsp = emu.snapshot().video;
    int ntXOffset = 0, ntYOffset = 0, mirXOffset = 0, mirYOffset = 0;
    switch (vsp->nt.mirror) {
    case ALDO_NTM_HORIZONTAL:
        ntYOffset = ntSize.y;
        mirXOffset = ntSize.x;
        break;
    case ALDO_NTM_VERTICAL:
        ntXOffset = ntSize.x;
        mirYOffset = ntSize.y;
        break;
    default:
        return;
    }
    auto ren = mr.renderer();
    auto target = atTex.asTarget(ren);
    for (auto i = 0; i < LayoutDim; ++i) {
        attr_span attributes = vsp->nt.tables[i].attributes;
        auto col = 0, row = 0;
        for (auto attr : attributes) {
            // NOTE: last row only uses the top half of attributes
            auto mtCount = row == attrDim - 1
                            ? ALDO_METATILE_DIM
                            : metatileCount;
            for (auto m = 0; m < mtCount; ++m) {
                aldo::color_span::size_type pidx = (attr >> (m * 2)) & 0x3;
                assert(pidx < aldo::color_span::extent);
                auto colors = vsp->palettes.bg[pidx];
                // NOTE: use 3rd color in the palette to represent the
                // attribute metatile, similar to nesdev wiki example.
                auto cidx = colors[2];
                auto c = emu.palette().getColor(cidx);
                auto [r, g, b] = aldo::colors::rgb(c);
                auto
                    mtX = m % 2 == 1 ? metatileStride : 0,
                    mtY = m > 1 ? metatileStride : 0,
                    xOffset = (i * ntXOffset) + (col * AttributeDim) + mtX,
                    yOffset = (i * ntYOffset) + (row * AttributeDim) + mtY;
                SDL_Rect metatile{
                    xOffset,
                    yOffset,
                    metatileStride,
                    metatileStride,
                };
                SDL_SetRenderDrawColor(ren, static_cast<Uint8>(r),
                                       static_cast<Uint8>(g),
                                       static_cast<Uint8>(b),
                                       SDL_ALPHA_OPAQUE);
                SDL_RenderFillRect(ren, &metatile);
                if (mirXOffset > 0) {
                    metatile.x += mirXOffset;
                    SDL_RenderFillRect(ren, &metatile);
                } else if (mirYOffset > 0) {
                    metatile.y += mirYOffset;
                    SDL_RenderFillRect(ren, &metatile);
                }
            }
            if (++col >= attrDim) {
                col = 0;
                ++row;
            }
        }
    }
}
