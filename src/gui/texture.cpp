//
//  texture.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/12/24.
//

#include "texture.hpp"

#include "error.hpp"
#include "viewstate.hpp"

#include "imgui.h"

#include <concepts>

namespace
{

static_assert(std::same_as<Uint32, ImU32>,
              "SDL u32 type does not match ImGui u32 type");

ALDO_OWN
auto create_texture(SDL_Point size, SDL_TextureAccess access,
                    SDL_Renderer* ren)
{
    auto tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, access, size.x,
                                 size.y);
    if (!tex) throw aldo::SdlError{"SDL texture creation failure"};
    return tex;
}

}

//
// MARK: - Public Interface
//

aldo::Texture::Texture(SDL_Point size, SDL_TextureAccess access,
                       SDL_Renderer* ren)
: tex{create_texture(size, access, ren)} {}

aldo::Texture::~Texture()
{
    SDL_DestroyTexture(tex);
}

void aldo::Texture::render(float scale) const noexcept
{
    int w, h;
    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    ImGui::Image(tex, {w * scale, h * scale});
}

aldo::Texture::TextureData::TextureData(SDL_Texture& tex) : tex{tex}
{
    void* data;
    int pitch;
    if (SDL_LockTexture(&tex, nullptr, &data, &pitch) < 0) {
        throw aldo::SdlError{"Texture is not streamable"};
    }
    pixels = static_cast<Uint32*>(data);
    stride = pitch / static_cast<int>(sizeof *pixels);
}

aldo::Texture::TextureData::~TextureData()
{
    SDL_UnlockTexture(&tex);
}

aldo::BouncerScreen::BouncerScreen(SDL_Point resolution, SDL_Renderer* ren)
: tex{resolution, SDL_TEXTUREACCESS_TARGET, ren} {}

void aldo::BouncerScreen::draw(const aldo::viewstate& vs,
                               SDL_Renderer* ren) const noexcept
{
    const auto& bouncer = vs.bouncer;

    SDL_SetRenderTarget(ren, tex.raw());

    SDL_SetRenderDrawColor(ren, 0x0, 0xff, 0xff, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);

    SDL_SetRenderDrawColor(ren, 0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(ren, 30, 7, 50, 200);

    SDL_SetRenderDrawColor(ren, 0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);

    auto fulldim = bouncer.halfdim * 2;
    SDL_Rect pos{
        bouncer.pos.x - bouncer.halfdim,
        bouncer.pos.y - bouncer.halfdim,
        fulldim,
        fulldim,
    };
    SDL_RenderFillRect(ren, &pos);

    SDL_SetRenderTarget(ren, nullptr);
}
