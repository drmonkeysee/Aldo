//
//  texture.hpp
//  Aldo
//
//  Created by Brandon Stansbury on 9/12/24.
//

#ifndef Aldo_gui_texture_hpp
#define Aldo_gui_texture_hpp

#include "attr.hpp"

#include <SDL2/SDL.h>

namespace aldo
{

struct viewstate;

// TODO: template parameterize with access flags for interface selection
class Texture {
public:
    class TextureData;

    Texture(SDL_Point size, SDL_TextureAccess access, SDL_Renderer* ren);
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = delete;
    Texture& operator=(Texture&&) = delete;
    ~Texture();

    SDL_Texture* raw() const noexcept { return tex; }

    void render(float scale = 1.0f) const noexcept;

    TextureData lock() const { return *tex; }

    class ALDO_SIDEFX TextureData {
    public:
        TextureData(const TextureData&) = delete;
        TextureData& operator=(const TextureData&) = delete;
        TextureData(TextureData&&) = delete;
        TextureData& operator=(TextureData&&) = delete;
        ~TextureData();

        Uint32* pixels;
        int stride;

    private:
        friend Texture;

        TextureData(SDL_Texture& tex);

        SDL_Texture& tex;
    };

private:
    SDL_Texture* tex;
};

class BouncerScreen {
public:
    BouncerScreen(SDL_Point resolution, SDL_Renderer* ren);

    void draw(const viewstate& vs, SDL_Renderer* ren) const noexcept;
    void render() const noexcept { tex.render(); }

private:
    Texture tex;
};

}

#endif
