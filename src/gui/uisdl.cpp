//
//  uisdl.cpp
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#include "uisdl.hpp"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <SDL2/SDL.h>

#include <cassert>

namespace
{

SDL_Window* Window;
SDL_Renderer* Renderer;
SDL_Texture* BouncerTexture;
bool Running = true, ShowDemo;
struct bouncer {
    SDL_Point bounds, pos, velocity;
    int dim;
} Bouncer{{256, 240}, {256 / 2, 240 / 2}, {1, 1}, 50};

enum guicleanup {
    GUI_CLEANUP_ALL,
    GUI_CLEANUP_BOUNCER,
    GUI_CLEANUP_RENDERER,
    GUI_CLEANUP_WINDOW,
    GUI_CLEANUP_SDL,
};

// TODO: this all needs to be RAII
void ui_cleanup(guicleanup status) noexcept
{
    switch (status) {
    case GUI_CLEANUP_ALL:
        ImGui_ImplSDLRenderer_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        // NOTE: fallthrough
    case GUI_CLEANUP_BOUNCER:
        SDL_DestroyTexture(BouncerTexture);
        BouncerTexture = nullptr;
        // NOTE: fallthrough
    case GUI_CLEANUP_RENDERER:
        SDL_DestroyRenderer(Renderer);
        Renderer = nullptr;
        // NOTE: fallthrough
    case GUI_CLEANUP_WINDOW:
        SDL_DestroyWindow(Window);
        Window = nullptr;
        // NOTE: fallthrough
    default:
        SDL_Quit();
    }
}

void render_bouncer() noexcept
{
    SDL_SetRenderTarget(Renderer, BouncerTexture);
    SDL_SetRenderDrawColor(Renderer, 0x0, 0xff, 0xff, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(Renderer);

    SDL_SetRenderDrawColor(Renderer, 0x0, 0x0, 0xff, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(Renderer, 30, 7, 50, 200);

    SDL_SetRenderDrawColor(Renderer, 0xff, 0xff, 0x0, SDL_ALPHA_OPAQUE);
    const int halfDim = Bouncer.dim / 2;
    const SDL_Rect pos{
        Bouncer.pos.x - halfDim,
        Bouncer.pos.y - halfDim,
        Bouncer.dim,
        Bouncer.dim,
    };
    SDL_RenderFillRect(Renderer, &pos);
    SDL_SetRenderTarget(Renderer, nullptr);

    ImGui::Begin("Bouncer");
    const ImVec2 sz{(float)Bouncer.bounds.x, (float)Bouncer.bounds.y};
    ImGui::Image(BouncerTexture, sz);
    ImGui::End();
}

//
// UI Interface Implementation
//

[[nodiscard("error code")]]
int init_ui(const gui_platform& platform) noexcept
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL initialization failure: %s", SDL_GetError());
        return UI_ERR_LIBINIT;
    }

    const bool hidpi = platform.is_hidpi();
    SDL_Log("HIDPI: %d", hidpi);
    const char *const name = platform.appname();
    Window = SDL_CreateWindow(name ? name : "DisplayNameErr",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              1280, 800, hidpi ? SDL_WINDOW_ALLOW_HIGHDPI : 0);
    if (!Window) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL window creation failure: %s", SDL_GetError());
        ui_cleanup(GUI_CLEANUP_SDL);
        return UI_ERR_LIBINIT;
    }

    Renderer = SDL_CreateRenderer(Window, -1,
                                  SDL_RENDERER_ACCELERATED
                                  | SDL_RENDERER_PRESENTVSYNC
                                  | SDL_RENDERER_TARGETTEXTURE);
    if (!Renderer) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "SDL renderer creation failure: %s",
                        SDL_GetError());
        ui_cleanup(GUI_CLEANUP_WINDOW);
        return UI_ERR_LIBINIT;
    }
    const float render_scale_factor = platform.render_scale_factor(Window);
    SDL_RenderSetScale(Renderer, render_scale_factor, render_scale_factor);
    SDL_RendererInfo info;
    SDL_GetRendererInfo(Renderer, &info);
    SDL_Log("Render info: %s (%04X) (x%.1f)", info.name, info.flags,
            render_scale_factor);

    BouncerTexture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_TARGET,
                                       Bouncer.bounds.x,
                                       Bouncer.bounds.y);
    if (!BouncerTexture) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "Bouncer texture creation failure: %s",
                        SDL_GetError());
        ui_cleanup(GUI_CLEANUP_RENDERER);
        return UI_ERR_LIBINIT;
    }

    if (!IMGUI_CHECKVERSION()) {
        ui_cleanup(GUI_CLEANUP_BOUNCER);
        return UI_ERR_LIBINIT;
    }

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(Window, Renderer);
    ImGui_ImplSDLRenderer_Init(Renderer);

    ImGui::StyleColorsDark();

    return 0;
}

void handle_input() noexcept
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        ImGui_ImplSDL2_ProcessEvent(&ev);
        if (ev.type == SDL_QUIT) {
            Running = false;
        }
    }
}

void update_stuff() noexcept
{
    const int halfdim = Bouncer.dim / 2;
    if (Bouncer.pos.x - halfdim < 0
        || Bouncer.pos.x + halfdim > Bouncer.bounds.x) {
        Bouncer.velocity.x *= -1;
    }
    if (Bouncer.pos.y - halfdim < 0
        || Bouncer.pos.y + halfdim > Bouncer.bounds.y) {
        Bouncer.velocity.y *= -1;
    }
    Bouncer.pos.x += Bouncer.velocity.x;
    Bouncer.pos.y += Bouncer.velocity.y;
}

void render_ui() noexcept
{
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Support")) {
            ImGui::MenuItem("ImGui Demo", nullptr, &ShowDemo);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (ShowDemo) {
        ImGui::ShowDemoWindow();
    }
    ImGui::Begin("First Window");
    ImGui::Text("Hello From Aldo+Dear ImGui");
    ImGui::End();

    render_bouncer();

    ImGui::Render();
    SDL_SetRenderDrawColor(Renderer, 0x1e, 0x1e, 0x1e, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(Renderer);
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(Renderer);
}

void cleanup_ui() noexcept
{
    ui_cleanup(GUI_CLEANUP_ALL);
}

void sdl_loop(nes*, struct console_state* snapshot) noexcept
{
    //assert(console != nullptr);
    assert(snapshot != nullptr);

    do {
        handle_input();
        if (Running) {
            update_stuff();
            render_ui();
        }
    } while (Running);
    cleanup_ui();
}

}

//
// Public Interface
//

int aldo::ui_sdl_init(const struct cliargs* args,
                      const struct gui_platform* platform,
                      ui_loop** loop) noexcept
{
    assert(args != nullptr);
    assert(platform != nullptr);
    assert(loop != nullptr);

    const int err = init_ui(*platform);
    *loop = err == 0 ? sdl_loop : nullptr;
    return err;
}
