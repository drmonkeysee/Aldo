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
#include "style.hpp"

#include <array>
#include <concepts>
#include <cassert>

static_assert(std::same_as<Uint32, ImU32>,
              "SDL u32 type does not match ImGui u32 type");

namespace
{

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
            auto tileRow = 0;
            for (auto row : tile) {
                auto rowOffset = (tblCol * ALDO_CHR_TILE_DIM)
                                    + ((tileRow++
                                        + (tblRow * ALDO_CHR_TILE_DIM))
                                       * data.stride);
                drawTileRow(row, colors, rowOffset, p, data);
            }
        }
    }
}

aldo::Nametables::Nametables(SDL_Point nametableSize,
                             const aldo::MediaRuntime& mr)
: ntSize{nametableSize}, tex{nametableSize * LayoutDim, mr.renderer()} {}

void aldo::Nametables::draw(aldo::color_span colors, const aldo::Palette& p,
                            const aldo::MediaRuntime& mr) const
{
    static_assert(colors.size() == NtCount, "mismatched nt sizes");

    auto ren = mr.renderer();
    auto target = tex.asTarget(ren);
    auto i = 0;
    for (auto cidx : colors) {
        auto c = p.getColor(cidx);
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
                                     const aldo::texture::TextureData& data)
{
    for (auto px = 0; px < ALDO_CHR_TILE_DIM; ++px) {
        auto pidx = ALDO_CHR_TILE_STRIDE - ((px + 1) * 2);
        assert(0 <= pidx);
        decltype(colors)::size_type texel = (row & (0x3 << pidx)) >> pidx;
        assert(texel < ALDO_PAL_SIZE);
        auto texidx = px + texOffset;
        assert(texidx < TextureDim * TextureDim);
        data.pixels[texidx] = p.getColor(colors[texel]);
    }
}
