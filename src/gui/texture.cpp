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
#include <type_traits>
#include <utility>
#include <cassert>

static_assert(std::same_as<Uint32, ImU32>,
              "SDL u32 type does not match ImGui u32 type");

namespace
{

using snp_video = decltype(std::declval<aldo_snapshot>().video);
using sprite_obj = std::remove_extent_t<
                    decltype(std::declval<snp_video>()->sprites.objects)>;
using sprite_span = std::span<const sprite_obj, AldoSpriteCount>;

constexpr auto SpriteDim = 256;

constexpr auto operator*(SDL_Point p, int n) noexcept
{
    return SDL_Point{p.x * n, p.y * n};
}

[[maybe_unused]]
auto screen_buffer_length() noexcept
{
    auto p = aldo::Emulator::screenSize();
    return p.x * p.y;
}

class Tile {
public:
    Tile(aldo::pt_tile chr, int col, int row, aldo::color_span c,
         const aldo::Palette& p, const aldo::tex::TextureData& d)
    : chrTile{chr}, colors{c}, palette{p}, data{d}, gridX{col}, gridY{row} {}
    Tile(aldo::pt_tile, int, int, aldo::color_span,
         aldo::Palette&&, const aldo::tex::TextureData&) = delete;
    Tile(aldo::pt_tile, int, int, aldo::color_span,
         const aldo::Palette&, aldo::tex::TextureData&&) = delete;
    Tile(aldo::pt_tile, int, int, aldo::color_span,
         aldo::Palette&&, aldo::tex::TextureData&&) = delete;

    void draw() const
    {
        auto chrDim = static_cast<int>(chrTile.size());
        auto chrRow = 0;
        for (auto pxRow : chrTile) {
            auto rowOrigin = origin
                                + (gridX * chrDim)
                                + ((chrRow++ + (gridY * chrDim)) * data.stride);
            drawRow(pxRow, rowOrigin);
        }
    }

    int origin = 0;

private:
    void drawRow(aldo::et::word pxRow, int rowOrigin) const
    {
        for (auto px = 0; px < static_cast<int>(decltype(chrTile)::extent); ++px) {
            auto pidx = AldoChrTileStride - ((px + 1) * 2);
            assert(0 <= pidx);
            decltype(colors)::size_type texel = (pxRow & (0x3 << pidx)) >> pidx;
            assert(texel < colors.size());
            auto texidx = px + rowOrigin;
            assert(texidx < data.size());
            data.pixels[texidx] = palette.getColor(colors[texel]);
        }
    }

    aldo::pt_tile chrTile;
    aldo::color_span colors;
    const aldo::Palette& palette;
    const aldo::tex::TextureData& data;
    int gridX, gridY;
};

auto draw_object(const sprite_obj& obj, snp_video v, const aldo::Palette& p,
                 const aldo::tex::TextureData& data)
{
    static constexpr auto
        palMin = static_cast<aldo::et::byte>(aldo::color_span::extent),
        palMax = static_cast<aldo::et::byte>((aldo::color_span::extent * 2) - 1);
    static constexpr int tileDim = aldo::pt_tile::extent;

    // Clamp uninitialized data within palette range; note that sprite palette
    // index is always in the upper half of the 8 available palettes.
    auto palidx = std::max(palMin, std::min(obj.palette, palMax));
    aldo::color_span colors = v->palettes.fg[palidx - decltype(colors)::extent];

    // TODO: clamp during initial testing
    auto x = std::min(static_cast<aldo::et::byte>(248), obj.x);
    auto y = std::min(static_cast<aldo::et::byte>(248), obj.y);
    auto gridCol = x / tileDim;
    auto gridRow = y / tileDim;
    auto gridXOffset = x % tileDim;
    auto gridYOffset = y % tileDim;
    auto offset = gridXOffset + (gridYOffset * data.stride);

    aldo::pt_span table = obj.pt ? v->pattern_tables.right : v->pattern_tables.left;
    Tile tile{table[obj.tile], gridCol, gridRow, colors, p, data};
    tile.origin = offset;
    tile.draw();
}

}

//
// MARK: - Public Interface
//

aldo::VideoScreen::VideoScreen(SDL_Point resolution, const aldo::MediaRuntime& mr)
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
            Tile tile{table[tileIdx], col, row, colors, p, data};
            tile.draw();
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

aldo::Sprites::Sprites(const aldo::MediaRuntime& mr)
: sprTex{{SpriteDim, SpriteDim}, mr.renderer()} {}

void aldo::Sprites::draw(const aldo::Emulator& emu) const
{
    using px_span = std::span<Uint32>;

    auto data = sprTex.lock();
    auto mem = px_span{data.pixels, static_cast<px_span::size_type>(data.size())};
    std::ranges::fill(mem, aldo::colors::LedOff);

    auto video = emu.snapshot().video;
    sprite_span objects = video->sprites.objects;
    for (const auto& obj : objects) {
        draw_object(obj, video, emu.palette(), data);
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
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_PIXELART);
    return tex;
}

aldo::tex::TextureData::TextureData(SDL_Texture& tex) noexcept : tex{tex}
{
    auto props = aldo::SdlProperties::texture(&tex);
    assert(SDL_PIXELTYPE(props.number(SDL_PROP_TEXTURE_FORMAT_NUMBER))
           == SDL_PIXELTYPE_PACKED32);
    width = props.number(SDL_PROP_TEXTURE_WIDTH_NUMBER);
    height = props.number(SDL_PROP_TEXTURE_HEIGHT_NUMBER);

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
    using nt_span = std::span<const aldo::et::byte, AldoNtTileCount>;

    auto vsp = emu.snapshot().video;
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
                auto tileId = tiles[tileIdx];
                auto colors = lookupTilePalette(attrs, col, row,
                                                vsp->palettes.bg);
                Tile tile{chrs[tileId], col, row, colors, emu.palette(), data};
                // account for upper NT bank X/Y offset for tiles
                tile.origin = (i * offsets.upperX)
                                + (i * offsets.upperY * data.stride);
                tile.draw();
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
    auto vsp = emu.snapshot().video;
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
    // last row only uses the top half of attributes
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
    // Use 3rd color in the palette to represent the
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
