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

using nt_span = std::span<const aldo::et::byte, AldoNtTileCount>;

constexpr auto operator*(SDL_Point p, int n) noexcept
{
    return SDL_Point{p.x * n, p.y * n};
}

auto screen_buffer_length() noexcept
{
    auto p = aldo::Emulator::screenSize();
    return p.x * p.y;
}

auto draw_tile_row(aldo::et::word pxRow, aldo::color_span colors,
                   int texOffset, const aldo::Palette& p,
                   const aldo::tex::TextureData& data)
{
    for (auto px = 0; px < static_cast<int>(aldo::pt_tile::extent); ++px) {
        auto pidx = AldoChrTileStride - ((px + 1) * 2);
        assert(0 <= pidx);
        decltype(colors)::size_type texel = (pxRow & (0x3 << pidx)) >> pidx;
        assert(texel < colors.size());
        auto texidx = px + texOffset;
        assert(texidx < data.size());
        data.pixels[texidx] = p.getColor(colors[texel]);
    }
}

auto draw_tile(aldo::pt_tile chr, int col, int row, aldo::color_span colors,
               const aldo::Palette& p, const aldo::tex::TextureData& data,
               int offsetAdjustment = 0)
{
    auto chrDim = static_cast<int>(chr.size());
    auto chrRow = 0;
    for (auto pxRow : chr) {
        auto texOffset = (col * chrDim) + ((chrRow++ + (row * chrDim))
                                           * data.stride);
        texOffset += offsetAdjustment;
        draw_tile_row(pxRow, colors, texOffset, p, data);
    }
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
    assert(data.size() == screen_buffer_length());
    for (auto i = 0; i < data.size(); ++i) {
        data.pixels[i] = p.getColor(vbuf[i]);
    }
}

aldo::PatternTable::PatternTable(const aldo::MediaRuntime& mr)
: tex{{TextureDim, TextureDim}, mr.renderer()} {}

void aldo::PatternTable::draw(aldo::pt_span table, aldo::color_span colors,
                              const aldo::Palette& p) const
{
    auto data = tex.lock();
    for (auto row = 0; row < TableDim; ++row) {
        for (auto col = 0; col < TableDim; ++col) {
            auto tileIdx = static_cast<
                decltype(table)::size_type>(col + (row * TableDim));
            draw_tile(table[tileIdx], col, row, colors, p, data);
        }
    }
}

aldo::Nametables::Nametables(SDL_Point nametableSize,
                             const aldo::MediaRuntime& mr)
: ntSize{nametableSize}, texSize{nametableSize * AldoNtCount},
ntTex{texSize, mr.renderer()}, atTex{texSize, mr.renderer()} {}

void aldo::Nametables::draw(const Emulator& emu, const MediaRuntime& mr) const
{
    if (mode == DrawMode::attributes) {
        drawAttributes(emu, mr);
    } else {
        drawNametables(emu);
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
    auto props = SDL_GetTextureProperties(&tex);
    assert(SDL_PIXELTYPE(SDL_GetNumberProperty(props,
                                               SDL_PROP_TEXTURE_FORMAT_NUMBER,
                                               0))
           == SDL_PIXELTYPE_PACKED32);
    width = SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_WIDTH_NUMBER, 0);
    height = SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_HEIGHT_NUMBER, 0);

    void* data;
    int pitch;
    SDL_LockTexture(&tex, nullptr, &data, &pitch);
    pixels = static_cast<Uint32*>(data);
    stride = pitch / static_cast<int>(sizeof *pixels);
}

//
// MARK: - Private Interface
//

void aldo::Nametables::drawNametables(const aldo::Emulator& emu) const
{
    const auto* vsp = emu.snapshot().video;
    aldo::pt_span chrs = vsp->nt.pt
                            ? vsp->pattern_tables.right
                            : vsp->pattern_tables.left;
    auto offsets = getOffsets(vsp->nt.mirror);

    auto data = ntTex.lock();
    for (auto i = 0; i < AldoNtCount; ++i) {
        nt_span tiles = vsp->nt.tables[i].tiles;
        attr_span attrs = vsp->nt.tables[i].attributes;
        for (auto row = 0; row < AldoNtHeight; ++row) {
            for (auto col = 0; col < AldoNtWidth; ++col) {
                auto tileIdx = static_cast<
                    decltype(tiles)::size_type>(col + (row * AldoNtWidth));
                auto tile = tiles[tileIdx];
                auto colors = lookupTilePalette(attrs, col, row,
                                                vsp->palettes.bg);
                // NOTE: account for upper NT bank X/Y offset for tiles
                auto offsetAdj = (i * offsets.upperX)
                                    + (i * offsets.upperY * data.stride);
                draw_tile(chrs[tile], col, row, colors, emu.palette(), data,
                          offsetAdj);
            }
        }
    }
    if (offsets.mirrorX > 0) {
        for (auto row = 0; row < texSize.y; ++row) {
            auto origin = data.pixels + (row * data.stride);
            std::copy(origin, origin + ntSize.x, origin + offsets.mirrorX);
        }
    }
    if (offsets.mirrorY > 0) {
        for (auto row = 0; row < ntSize.y; ++row) {
            auto origin = data.pixels + (row * data.stride);
            std::copy(origin, origin + texSize.x,
                      origin + (offsets.mirrorY * data.stride));
        }
    }
}

void aldo::Nametables::drawAttributes(const aldo::Emulator& emu,
                                      const aldo::MediaRuntime& mr) const
{
    const auto* vsp = emu.snapshot().video;
    auto offsets = getOffsets(vsp->nt.mirror);
    auto ren = mr.renderer();
    auto target = atTex.asTarget(ren);
    for (auto i = 0; i < AldoNtCount; ++i) {
        attr_span attrs = vsp->nt.tables[i].attributes;
        for (auto row = 0; row < AttributeDim; ++row) {
            for (auto col = 0; col < AttributeDim; ++col) {
                drawAttribute(attrs, i, col, row, offsets, vsp->palettes.bg,
                              emu.palette(), ren);
            }
        }
    }
}

aldo::Nametables::nt_offsets
aldo::Nametables::getOffsets(aldo_ntmirror m) const noexcept
{
    nt_offsets offsets;
    switch (m) {
    case ALDO_NTM_HORIZONTAL:
        offsets.upperY = ntSize.y;
        offsets.mirrorX = ntSize.x;
        break;
    case ALDO_NTM_VERTICAL:
        offsets.upperX = ntSize.x;
        offsets.mirrorY = ntSize.y;
        break;
    default:
        break;
    }
    return offsets;
}

aldo::color_span
aldo::Nametables::lookupTilePalette(attr_span attrs, int tileCol, int tileRow,
                                    pal_span palettes) noexcept
{
    using ps_sz = decltype(palettes)::size_type;

    auto attrIdx = static_cast<
        decltype(attrs)::size_type>((tileCol >> AldoMetatileDim)
                                    + ((tileRow >> AldoMetatileDim)
                                       * AttributeDim));
    auto attr = attrs[attrIdx];
    auto mtIdx = static_cast<ps_sz>(((tileCol >> 1) % AldoMetatileDim)
                                    + (((tileRow >> 1) % AldoMetatileDim)
                                       * AldoMetatileDim));
    assert(mtIdx < palettes.size());
    ps_sz palIdx = (attr >> (mtIdx * 2)) & 0x3;
    assert(palIdx < palettes.size());
    return palettes[palIdx];
}

void aldo::Nametables::drawAttribute(attr_span attrs, int ntIdx, int col,
                                     int row, const nt_offsets& offsets,
                                     pal_span bg, const aldo::Palette& p,
                                     SDL_Renderer* ren)
{
    auto attrIdx = static_cast<
        decltype(attrs)::size_type>(col + (row * AttributeDim));
    auto attr = attrs[attrIdx];
    // NOTE: last row only uses the top half of attributes
    auto mtCount = row == AttributeDim - 1 ? AldoMetatileDim : MetatileCount;
    for (auto m = 0; m < mtCount; ++m) {
        drawMetatile(attr, ntIdx, col, row, m, offsets, bg, p, ren);
    }
}

void aldo::Nametables::drawMetatile(aldo::et::byte attr, int ntIdx, int col,
                                    int row, int metaTile,
                                    const nt_offsets& offsets, pal_span bg,
                                    const aldo::Palette& p, SDL_Renderer* ren)
{
    static constexpr auto metatileStride = AldoMetatileDim * TilePxDim;

    aldo::color_span::size_type pidx = (attr >> (metaTile * 2)) & 0x3;
    assert(pidx < aldo::color_span::extent);
    aldo::color_span colors = bg[pidx];
    // NOTE: use 3rd color in the palette to represent the
    // attribute metatile, similar to nesdev wiki example.
    auto cidx = colors[2];
    auto c = p.getColor(cidx);
    auto [r, g, b] = aldo::colors::rgb(c);
    auto
        mtX = metaTile % 2 == 1 ? metatileStride : 0,
        mtY = metaTile > 1 ? metatileStride : 0,
        xOffset = (ntIdx * offsets.upperX) + (col * AttributePxDim) + mtX,
        yOffset = (ntIdx * offsets.upperY) + (row * AttributePxDim) + mtY;

    SDL_FRect metatile{
        static_cast<float>(xOffset),
        static_cast<float>(yOffset),
        metatileStride,
        metatileStride,
    };
    SDL_SetRenderDrawColor(ren, static_cast<Uint8>(r), static_cast<Uint8>(g),
                           static_cast<Uint8>(b), SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(ren, &metatile);
    if (offsets.mirrorX > 0) {
        metatile.x += offsets.mirrorX;
        SDL_RenderFillRect(ren, &metatile);
    } else if (offsets.mirrorY > 0) {
        metatile.y += offsets.mirrorY;
        SDL_RenderFillRect(ren, &metatile);
    }
}
